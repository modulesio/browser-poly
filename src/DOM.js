const path = require('path');
const fs = require('fs');
const url = require('url');
const vm = require('vm');
const util = require('util');
const {parentPort} = require('worker_threads');

const {process} = global;

const css = require('css');
const he = require('he');
const parse5 = require('parse5');
const parseIntStrict = require('parse-int');
const selector = require('window-selector');
const {fetch} = require('./fetch');
const {Blob} = fetch;
const htmlUnescape = require('unescape');

const bindings = require('./native-bindings');
const {Event, EventTarget, MessageEvent, MouseEvent, ErrorEvent} = require('./Event');
const {_makeWindow} = require('./WindowVm');
const GlobalContext = require('./GlobalContext');
const symbols = require('./symbols');
const {_elementGetter, _elementSetter, _normalizeUrl} = require('./utils');
const {XRRigidTransform} = require('./XR');
const {ElectronVm} = require('./electron-vm.js');

he.encode.options.useNamedReferences = true;

const _promiseSerial = async promiseFns => {
  for (let i = 0; i < promiseFns.length; i++) {
    await promiseFns[i]();
  }
};
const _loadPromise = el => new Promise((accept, reject) => {
  const load = () => {
    _cleanup();
    accept();
  };
  const error = err => {
    _cleanup();
    reject(err);
  };
  const _cleanup = () => {
    el.removeListener('load', load);
    el.removeListener('error', error);
  };
  el.on('load', load);
  el.on('error', error);
});
const _windowHandleEquals = (a, b) => {
  if (a && b) {
    return a[0] === b[0] && a[1] === b[1];
  } else {
    return !a && !b;
  }
};

const EMPTY_ARRAY = [];

class DOMRect {
  constructor(x = 0, y = 0, w = 0, h = 0) {
    this.x = x;
    this.y = y;
    this.width = w;
    this.height = h;
    this.left = w >= 0 ? x : x + w;
    this.top = h >= 0 ? y : y + h;
    this.right = w >= 0 ? x + w : x;
    this.bottom = h >= 0 ? y + h : y;
  }
}
module.exports.DOMRect = DOMRect;

class DOMPoint {
  constructor(x = 0, y = 0, z = 0, w = 1) {
    if (typeof x === 'object') {
      this.values = x;
    } else {
      this.values = new Float32Array(4);
      this.x = x;
      this.y = y;
      this.z = z;
      this.w = w;
    }
  }
  get x() {
    return this.values[0];
  }
  set x(x) {
    this.values[0] = x;
  }
  get y() {
    return this.values[1];
  }
  set y(y) {
    this.values[1] = y;
  }
  get z() {
    return this.values[2];
  }
  set z(z) {
    this.values[2] = z;
  }
  get w() {
    return this.values[3];
  }
  set w(w) {
    this.values[3] = w;
  }
}
module.exports.DOMPoint = DOMPoint;
GlobalContext.DOMPoint = DOMPoint;

class NodeList extends Array {
  constructor(nodes) {
    super();

    if (Array.isArray(nodes)) {
      this.push.apply(this, nodes);
    }
  }

  item(k) {
    const v = this[k];
    return v !== undefined ? v : null;
  }
}
module.exports.NodeList = NodeList;

class HTMLCollection extends Array {
  constructor(nodes) {
    super();

    if (Array.isArray(nodes)) {
      this.push.apply(this, nodes);
    }
  }
}
module.exports.HTMLCollection = HTMLCollection;

class Node extends EventTarget {
  constructor(window) {
    super();

    this.parentNode = null;
    this.childNodes = new NodeList();
    this.ownerDocument = window.document;

    this[symbols.windowSymbol] = window;
  }

  get parentElement() {
    if (this.parentNode && this.parentNode.nodeType === Node.ELEMENT_NODE) {
      return this.parentNode;
    } else {
      return null;
    }
  }
  set parentElement(parentElement) {}

  get nextSibling() {
    if (this.parentNode) {
      const selfIndex = this.parentNode.childNodes.indexOf(this);
      const nextIndex = selfIndex + 1;
      if (nextIndex < this.parentNode.childNodes.length) {
        return this.parentNode.childNodes[nextIndex];
      } else {
        return null;
      }
    } else {
      return null;
    }
  }
  set nextSibling(nextSibling) {}
  get previousSibling() {
    if (this.parentNode) {
      const selfIndex = this.parentNode.childNodes.indexOf(this);
      const prevIndex = selfIndex - 1;
      if (prevIndex >= 0) {
        return this.parentNode.childNodes[prevIndex];
      } else {
        return null;
      }
    } else {
      return null;
    }
  }
  set previousSibling(previousSibling) {}

  get nextElementSibling() {
    if (this.parentNode) {
      const selfIndex = this.parentNode.childNodes.indexOf(this);
      for (let i = selfIndex + 1; i < this.parentNode.childNodes.length; i++) {
        const childNode = this.parentNode.childNodes[i];
        if (childNode.nodeType === Node.ELEMENT_NODE) {
          return childNode;
        }
      }
      return null;
    } else {
      return null;
    }
  }
  set nextElementSibling(nextElementSibling) {}
  get previousElementSibling() {
    if (this.parentNode) {
      const selfIndex = this.parentNode.childNodes.indexOf(this);
      for (let i = selfIndex - 1; i >= 0; i--) {
        const childNode = this.parentNode.childNodes[i];
        if (childNode.nodeType === Node.ELEMENT_NODE) {
          return childNode;
        }
      }
      return null;
    } else {
      return null;
    }
  }
  set previousElementSibling(previousElementSibling) {}

  get nodeValue() {
    return null;
  }
  set nodeValue(nodeValue) {}

  contains(el) {
    for (;;) {
      if (el === this) {
        return true;
      } else if (el.parentNode) {
        el = el.parentNode;
      } else {
        return false;
      }
    }
  }

  get isConnected() {
    for (let el = this; el; el = el.parentNode) {
      if (el.parentNode === el.ownerDocument) {
        return true;
      }
    }
    return false;
  }
  set isConnected(isConnected) {}

  cloneNode(deep = false) {
    return _cloneNode(deep, this);
  }

  _dispatchEventOnDocumentReady() {
    if (this.ownerDocument.readyState === 'complete') {
      this.dispatchEvent.apply(this, arguments);
    } else {
      const args = Array.from(arguments);

      const _readystatechange = () => {
        if (this.ownerDocument.readyState === 'complete') {
          this.ownerDocument.removeEventListener('readystatechange', _readystatechange);

          process.nextTick(() => {
            const el = args[0].target;
            el.dispatchEvent.apply(el, args);
          });
        }
      };
      this.ownerDocument.addEventListener('readystatechange', _readystatechange);
    }
  }
}

/**
 * Clone node. Internal function to not expose the `sourceNode` and `parentNode` args
 * used to facilitate recursive cloning.
 *
 * @param {boolean} deep - Recursive.
 * @param {object} sourceNode - Node to clone.
 * @param {parentNode} parentNode - Used for recursive cloning to attach parent.
 */
function _cloneNode(deep, sourceNode, parentNode) {
  const clone = new sourceNode.constructor(sourceNode[symbols.windowSymbol]);
  clone.attrs = sourceNode.attrs;
  clone.tagName = sourceNode.tagName;
  clone.value = sourceNode.value;

  // Link the parent.
  if (parentNode) { clone.parentNode = parentNode; }

  // Copy children.
  if (deep) {
    clone.childNodes = new NodeList(
      sourceNode.childNodes.map(childNode =>
        _cloneNode(true, childNode, clone)
      )
    );
  }

  return clone;
}

Node.ELEMENT_NODE = 1;
Node.TEXT_NODE = 3;
Node.PROCESSING_INSTRUCTION_NODE = 7;
Node.COMMENT_NODE = 8;
Node.DOCUMENT_NODE = 9;
Node.DOCUMENT_TYPE_NODE = 10;
Node.DOCUMENT_FRAGMENT_NODE = 11;
module.exports.Node = Node;

const _setAttributeRaw = (el, prop, value) => {
  if (prop === 'length') {
    el.attrs.length = value;
  } else {
    const attr = el.attrs.find(attr => attr.name === prop);
    if (!attr) {
      const attr = {
        name: prop,
        value,
      };
      el.attrs.push(attr);
      el._emit('attribute', prop, value, null);
    } else {
      const oldValue = attr.value;
      attr.value = value;
      el._emit('attribute', prop, value, oldValue);
    }
  }
};

const _makeAttr = attr => attr && ({ // XXX should be class Attr
  name: attr.name,
  value: attr.value,
  nodeName: attr.name,
  nodeValue: attr.value,
});
const _makeAttributesProxy = el => new Proxy(el.attrs, {
  get(target, prop) {
    if (prop === Symbol.iterator) {
      const {length} = target;
      let i = 0;
      return () => ({
        next() {
          return i < length ? {
            value: _makeAttr(target[i++]),
            done: false,
          } : {
            done: true,
          };
        },
      });
    } else {
      const propN = parseIntStrict(prop);
      if (propN !== undefined) {
        return _makeAttr(target[propN]);
      } else if (prop === 'length') {
        return target.length;
      } else {
        return _makeAttr(target.find(attr => attr.name === prop));
      }
    }
  },
  set(target, prop, value) {
    _setAttributeRaw(el, prop, value);
    return true;
  },
  deleteProperty(target, prop) {
    const index = target.findIndex(attr => attr.name === prop);
    if (index !== -1) {
      const oldValue = target[index].value;
      target.splice(index, 1);
      el._emit('attribute', prop, null, oldValue);
    }
    return true;
  },
  has(target, prop) {
    if (typeof prop === 'number') {
      return target[prop] !== undefined;
    } else if (prop === 'length') {
      return true;
    } else {
      return target.findIndex(attr => attr.name === prop) !== -1;
    }
  },
});

const _makeChildrenProxy = el => {
  const {HTMLElement} = el.ownerDocument.defaultView;

  const result = [];
  result.item = i => {
    if (typeof i === 'number') {
      return result[i];
    } else {
      return undefined;
    }
  };
  result.update = () => {
    result.length = 0;

    for (let i = 0; i < el.childNodes.length; i++) {
      const childNode = el.childNodes[i];
      if (childNode instanceof HTMLElement) {
        result.push(childNode);
      }
    }
  };
  result.update();
  return result;
};

const _cssText = style => {
  let styleString = '';
  for (const k in style) {
    const v = style[k];
    if (v !== undefined) {
      styleString += (styleString.length > 0 ? ' ' : '') + k + ': ' + v + ';';
    }
  }
  return styleString;
};

const _makeStyleProxy = el => {
  const style = {};
  let needsReset = true;
  const _reset = () => {
    needsReset = false;
    let stylesheet, err;
    try {
      stylesheet = css.parse(`x{${el.getAttribute('style')}}`).stylesheet;
    } catch(e) {
      err = e;
    }
    if (!err) {
      for (const k in style) {
        delete style[k];
      }
      const {rules} = stylesheet;
      for (let j = 0; j < rules.length; j++) {
        const rule = rules[j];
        const {declarations} = rule;
        for (let k = 0; k < declarations.length; k++) {
          let {property, value} = declarations[k];
          if (/^\.[0-9]+[a-z]*$/i.test(value)) {
            value = '0' + value;
          }
          style[property] = value;
        }
      }
    }
  };
  const _getValue = (style, key) => {
    if (needsReset) {
      _reset();
    }
    return style[key];
  };
  const _setValue = (style, key, value) => {
    if (key === 'cssText') {
      el.setAttribute('style', value);
    } else {
      style[key] = value;
      el.setAttribute('style', _cssText(style));
    }
  };
  const _removeValue = (style, key, value) => {
    delete style[key];
    el.setAttribute('style', _cssText(style));
  };
  const _makeProxy = (style, getValue, setValue, removeValue) => new Proxy({}, {
    get(target, key) {
      if (key === 'reset') {
        return _reset;
      } else if (key === 'clone') {
        return () => {
          if (needsReset) {
            _reset();
          }

          const newStyle = {};
          for (const k in style) {
            const v = style[k];
            if (v !== undefined) {
              newStyle[k] = v;
            }
          }

          const _getValue = (style, key) => style[key];
          const _setValue = (style, key, value) => {
            style[key] = value;
          };
          const _removeValue = (style, key) => {
            delete style[value];
          };
          return _makeProxy(newStyle, _getValue, _setValue, _removeValue);
        };
      } else if (key === 'cssText') {
        return el.getAttribute('style') || '';
      } else if (key === 'length') {
        return Object.keys(style).length;
      } else if (key === 'getPropertyPriority') {
        return () => '';
      } else if (key === 'getPropertyValue') {
        return key => getValue(style, key);
      } else if (key === 'setProperty') {
        return (propertyName, value, priority) => {
          setValue(style, propertyName, value);
        };
      } else if (key === 'removeProperty') {
        return property => {
          removeValue(style, property);
        };
      } else if (key === 'item') {
        return k => {
          const n = parseInt(k, 10);
          if (!isNaN(n)) {
            const keys = Object.keys(style);
            const key = keys[n];
            return style[key];
          } else {
            return undefined;
          }
        };
      } else {
        return getValue(style, key);
      }
    },
    set(target, key, value) {
      setValue(style, key, value);
      return true;
    },
  });
  return _makeProxy(style, _getValue, _setValue, _removeValue);
};

const _dashToCamelCase = s => {
  let match = s.match(/^data-(.+)$/);
  if (match) {
    s = match[1];
    s = s.replace(/-([a-z])/g, (all, letter) => letter.toUpperCase());
    return s;
  } else {
    return null;
  }
};

const _camelCaseToDash = s => {
  if (!/-[a-z]/.test(s)) {
    s = 'data-' + s;
    s = s.replace(/([A-Z])/g, (all, letter) => '-' + letter.toLowerCase());
    return s;
  } else {
    return null;
  }
};

const _makeDataset = el => new Proxy(el, {
  get(target, key) {
    for (let i = 0; i < target.attrs.length; i++) {
      const attr = target.attrs[i];
      if (_dashToCamelCase(attr.name) === key) {
        return attr.value;
      }
    }
  },
  set(target, key, value) {
    const dashName = _camelCaseToDash(key);
    if (dashName) {
      _setAttributeRaw(target, dashName, value);
    }
    return true;
  },
  getOwnPropertyDescriptor(target, key) {
    for (let i = 0; i < target.attrs.length; i++) {
      const attr = target.attrs[i];
      if (_dashToCamelCase(attr.name) === key) {
        return Reflect.getOwnPropertyDescriptor(target.attrs, i);
      }
    }
  },
  ownKeys(target) {
    const result = [];
    for (let i = 0; i < target.attrs.length; i++) {
      const camelCase = _dashToCamelCase(target.attrs[i].name);
      if (camelCase !== null) {
        result.push(camelCase);
      }
    }
    return result;
  },
});

const autoClosingTags = {
  area: true,
  base: true,
  br: true,
  embed: true,
  hr: true,
  iframe: true,
  img: true,
  input: true,
  link: true,
  meta: true,
  param: true,
  source: true,
  track: true,
  window: true,
};

const _defineId = (window, id, el) => {
  let value;
  Object.defineProperty(window, id, {
    get() {
      if (value !== undefined) {
        return value;
      } else if (el.ownerDocument.documentElement.contains(el) && el.getAttribute('id') === id) {
        return el;
      }
    },
    set(newValue) {
      value = newValue;
    },
    configurable: true,
  });
};

class DOMTokenList extends Array {
  constructor(onchange) {
    super();

    this.onchange = onchange;
  }

  item(k) {
    const v = this[k];
    return v !== undefined ? v : null;
  }

  add() {
    for (let i = 0; i < arguments.length; i++) {
      const name = '' + arguments[i];

      if (this.indexOf(name) !== -1) {
        continue;
      }

      this.push(name);
    }

    this.onchange(this.toString());

    return this;
  }

  remove() {
    var index,
        name,
        i,
        j;

    for (i = 0; i < arguments.length; i += 1) {
      name = arguments[i] + '';
      index = this.indexOf(name);

      if (index < 0) {
        continue;
      }

      for (j = index; j < this.length - 1; j++) {
        this[index] = this[index + 1];
      }
      this.length--;
    }

    this.onchange(this.toString());

    return this;
  }

  contains(name) {
    name += '';
    return this.indexOf(name) !== -1;
  }

  toggle(name, force) {
    name += '';

    if (force === true) {
      return this.add(name);
    }
    if (force === false) {
      return this.remove(name);
    }

    return this[this.contains(name) ? 'remove' : 'add'](name);
  }

  toString() {
    return this.join(' ');
  }
}
module.exports.DOMTokenList = DOMTokenList;

const _resetClassList = (classList, className) => {
  classList.length = 0;

  const classes = className
    .replace(/^\s+|\s+$/g, '')
    .split(/\s+/);
  for (let i = 0; i < classes.length; i += 1) {
    if (classes[i]) {
      classList.push(classes[i]);
    }
  }
};

class Element extends Node {
  constructor(window, tagName = 'DIV', attrs = [], value = '', location = null) {
    super(window);

    this.tagName = tagName;
    this.attrs = attrs;
    this.value = value;
    this.location = location;

    this._attributes = null;
    this._children = null;
    this._innerHTML = '';
    this._classList = null;
    this._dataset = null;

    this.on('attribute', (name, value) => {
      if (name === 'id') {
        if (this.ownerDocument.defaultView[value] === undefined) {
          _defineId(this.ownerDocument.defaultView, value, this);
        }
      } else if (name === 'class' && this._classList) {
        _resetClassList(this._classList, value);
      }
    });
    this.on('children', (addedNodes, removedNodes, previousSibling, nextSiblings) => {
      for (let i = 0; i < addedNodes.length; i++) {
        addedNodes[i]._emit('attached');
      }
      for (let i = 0; i < removedNodes.length; i++) {
        removedNodes[i]._emit('removed');
      }
    });
  }

  get nodeType() {
    return Node.ELEMENT_NODE;
  }
  set nodeType(nodeType) {}

  get nodeName() {
    return this.tagName;
  }
  set nodeName(nodeName) {}

  get attributes() {
    if (!this._attributes) {
      this._attributes = _makeAttributesProxy(this);
    }
    return this._attributes;
  }
  set attributes(attributes) {}

  get children() {
    if (!this._children) {
      this._children = _makeChildrenProxy(this);
    }
    return this._children;
  }
  set children(children) {}

  getAttribute(name) {
    const attr = this.attributes[name];
    return attr !== undefined ? attr.value : null;
  }
  setAttribute(name, value) {
    value = value + '';
    this.attributes[name] = value;
  }
  setAttributeNS(namespace, name, value) {
    this.setAttribute(name, value);
  }
  hasAttribute(name) {
    return name in this.attributes;
  }
  removeAttribute(name) {
    const oldValue = this.attributes[name];
    delete this.attributes[name];
  }

  getAttributeNames() {
    return this.attrs.map(attr => attr.name);
  }

  appendChild(childNode) {
    var newChildren = [];

    if (childNode.nodeType === Node.DOCUMENT_FRAGMENT_NODE) {
      // Appending DocumentFragment, append all children from node.
      let fragment = childNode;
      for (let i = 0; i < fragment.childNodes.length; i++) {
        let newChildNode = fragment.childNodes[i].cloneNode(true);
        this.childNodes.push(newChildNode);
        newChildNode.parentNode = this;
        newChildren.push(newChildNode);
      }
    } else {
      // Normal appendChild.
      if (childNode.parentNode) {
        childNode.parentNode.removeChild(childNode);
      }
      this.childNodes.push(childNode);
      childNode.parentNode = this;
      newChildren.push(childNode);
    }

    if (this._children) { this._children.update(); }

    // Notify observers.
    this._emit('children', newChildren, EMPTY_ARRAY,
               this.childNodes[this.childNodes.length - 2] || null, null);
    this.ownerDocument._emit('domchange');

    return childNode;
  }

  removeChild(childNode) {
    const index = this.childNodes.indexOf(childNode);
    if (index !== -1) {
      this.childNodes.splice(index, 1);
      childNode.parentNode = null;

      if (this._children) {
        this._children.update();
      }

      this._emit('children', [], [childNode], this.childNodes[index - 1] || null, this.childNodes[index] || null);
      this.ownerDocument._emit('domchange');

      return childNode;
    } else {
      throw new Error('The node to be removed is not a child of this node.');
    }
  }

  append() {
    for (let i = 0; i < arguments.length; i++) {
      const content = arguments[i];
      if (typeof content === 'string') {
        this.appendChild(this.ownerDocument.createTextNode(content));
      } else {
        this.appendChild(content);
      }
    }
  }
  remove() {
    if (this.parentNode !== null) {
      this.parentNode.removeChild(this);
    }
  }

  replaceChild(newChild, oldChild) {
    const index = this.childNodes.indexOf(oldChild);
    if (index !== -1) {
      this.childNodes.splice(index, 1, newChild);
      oldChild.parentNode = null;

      if (this._children) {
        this._children.update();
      }

      this._emit('children', [newChild], [oldChild], this.childNodes[index - 1] || null, this.childNodes[index] || null);
      this.ownerDocument._emit('domchange');

      return oldChild;
    } else {
      throw new Error('The node to be replaced is not a child of this node.');
    }
  }
  insertBefore(childNode, nextSibling) {
    let index = this.childNodes.indexOf(nextSibling);
    if (index !== -1) {
      index = this.childNodes.length;
    }

    this.childNodes.splice(index, 0, childNode);
    childNode.parentNode = this;

    if (this._children) {
      this._children.update();
    }

    this._emit('children', [childNode], [], this.childNodes[index - 1] || null, this.childNodes[index + 1] || null);
    this.ownerDocument._emit('domchange');
  }
  insertAdjacentHTML(position, text) {
    const _getEls = text => parse5.parseFragment(text, {
      locationInfo: true,
    })
      .childNodes
      .map(childNode =>
        GlobalContext._fromAST(childNode, this.ownerDocument.defaultView, this, this.ownerDocument, true)
      );
    switch (position) {
      case 'beforebegin': {
        const index = this.parentNode.childNodes.indexOf(this);
        const newChildNodes = _getEls(text);
        this.parentNode.childNodes.splice.apply(this.parentNode.childNodes, [
          index,
          0,
        ].concat(newChildNodes));
        this.parentNode._emit('children', newChildNodes, [], null, null);
        break;
      }
      case 'afterbegin': {
        const newChildNodes = _getEls(text);
        this.childNodes.splice.apply(this.childNodes, [
          0,
          0,
        ].concat(newChildNodes));
        this._emit('children', newChildNodes, [], null, null);
        break;
      }
      case 'beforeend': {
        const newChildNodes = _getEls(text);
        this.childNodes.splice.apply(this.childNodes, [
          this.childNodes.length,
          0,
        ].concat(newChildNodes));
        this._emit('children', newChildNodes, [], null, null);
        break;
      }
      case 'afterend': {
        const index = this.parentNode.childNodes.indexOf(this);
        const newChildNodes = _getEls(text);
        this.parentNode.childNodes.splice.apply(this.parentNode.childNodes, [
          index + 1,
          0,
        ].concat(newChildNodes));
        this.parentNode._emit('children', newChildNodes, [], null, null);
        break;
      }
      default: {
        throw new Error('invalid position: ' + position);
        break;
      }
    }
  }

  get firstChild() {
    return this.childNodes.length > 0 ? this.childNodes[0] : null;
  }
  set firstChild(firstChild) {}
  get lastChild() {
    return this.childNodes.length > 0 ? this.childNodes[this.childNodes.length - 1] : null;
  }
  set lastChild(lastChild) {}

  get firstElementChild() {
    for (let i = 0; i < this.childNodes.length; i++) {
      const childNode = this.childNodes[i];
      if (childNode.nodeType === Node.ELEMENT_NODE) {
        return childNode;
      }
    }
    return null;
  }
  set firstElementChild(firstElementChild) {}
  get lastElementChild() {
    for (let i = this.childNodes.length - 1; i >= 0; i--) {
      const childNode = this.childNodes[i];
      if (childNode.nodeType === Node.ELEMENT_NODE) {
        return childNode;
      }
    }
    return null;
  }
  set lastElementChild(lastElementChild) {}

  get childElementCount() {
    let result = 0;
    for (let i = 0; i < this.childNodes.length; i++) {
      if (this.childNodes[i].nodeType === Node.ELEMENT_NODE) {
        result++;
      }
    }
    return result;
  }
  set childElementCount(childElementCount) {}

  get id() {
    return this.getAttribute('id') || '';
  }
  set id(id) {
    id = id + '';
    this.setAttribute('id', id);
  }

  get className() {
    return this.getAttribute('class') || '';
  }
  set className(className) {
    className = className + '';
    this.setAttribute('class', className);
  }

  get classList() {
    if (!this._classList) {
      this._classList = new DOMTokenList(className => {
        _setAttributeRaw(this, 'class', className);
      });
    }
    return this._classList;
  }
  set classList(classList) {}

  getElementById(id) {
    id = id + '';
    return selector.find(this, '#' + id, true);
  }
  getElementByClassName(className) {
    className = className + '';
    return selector.find(this, '.' + className, true);
  }
  getElementByTagName(tagName) {
    tagName = tagName + '';
    return selector.find(this, tagName, true);
  }
  querySelector(s) {
    s = s + '';
    return selector.find(this, s, true);
  }
  getElementsById(id) {
    id = id + '';
    return selector.find(this, '#' + id);
  }
  getElementsByClassName(className) {
    className = className + '';
    return selector.find(this, '.' + className);
  }
  getElementsByTagName(tagName) {
    tagName = tagName + '';
    return selector.find(this, tagName);
  }
  querySelectorAll(s) {
    s = s + '';
    return selector.find({
      traverse: fn => {
        for (let i = 0; i < this.childNodes.length; i++) {
          this.childNodes[i].traverse(fn);
        }
      },
    }, s);
  }
  matches(s) {
    s = s + '';
    return selector.matches(this, s);
  }
  closest(s) {
    for (let el = this; el; el = el.parentNode) {
      if (el.matches(s)) {
        return el;
      }
    }
    return null;
  }

  getBoundingClientRect() {
    return new DOMRect(0, 0, this.clientWidth, this.clientHeight);
  }

  focus() {
    const document = this.tagName === 'DOCUMENT' ? this : this.ownerDocument;
    document.activeElement.dispatchEvent(new Event('blur', {
      target: document.activeElement,
    }));

    document.activeElement = this;
    this.dispatchEvent(new Event('focus', {
      target: this,
    }));
  }

  blur() {
    const document = this.ownerDocument;
    if (document.activeElement !== document.body) {
      document.body.focus();
    }
  }

  click() {
    this.dispatchEvent(new MouseEvent('click'));
  }

  get clientWidth() {
    switch (this.tagName) {
      case 'DOCUMENT':
      case 'HTML':
      case 'BODY': {
        const ownerDocument = this.ownerDocument || this;
        return ownerDocument.defaultView.innerWidth;
      }
      case 'CANVAS':
      case 'IMAGE':
      case 'VIDEO':
        return this.width;
      default:
        return 0;
    }
  }
  set clientWidth(clientWidth) {}
  get clientHeight() {
    switch (this.tagName) {
      case 'DOCUMENT':
      case 'HTML':
      case 'BODY': {
        const ownerDocument = this.ownerDocument || this;
        return ownerDocument.defaultView.innerHeight;
      }
      case 'CANVAS':
      case 'IMAGE':
      case 'VIDEO':
        return this.height;
      default:
        return 0;
    }
  }
  set clientHeight(clientHeight) {}

  get dataset() {
    if (!this._dataset) {
      this._dataset = _makeDataset(this);
    }
    return this._dataset;
  }
  set dataset(dataset) {}

  get innerHTML() {
    return parse5.serialize(this);
  }
  set innerHTML(innerHTML) {
    innerHTML = innerHTML + '';
    const oldChildNodes = this.childNodes;
    const newChildNodes = new NodeList(
      parse5.parseFragment(innerHTML, {
        locationInfo: true,
      })
        .childNodes
        .map(childNode =>
          GlobalContext._fromAST(childNode, this.ownerDocument.defaultView, this, this.ownerDocument, true)
        )
    );
    this.childNodes = newChildNodes;

    if (this._children) {
      this._children.update();
    }

    this._emit('children', newChildNodes, oldChildNodes, null, null);
    this.ownerDocument._emit('domchange');

    _promiseSerial(newChildNodes.map(childNode => () => GlobalContext._runHtml(childNode, this.ownerDocument.defaultView)))
      .catch(err => {
        console.warn(err);
      });

    this._emit('innerHTML', innerHTML);
  }

  get innerText() {
    return he.encode(this.innerHTML);
  }
  set innerText(innerText) {
    innerText = innerText + '';
    this.innerHTML = he.decode(innerText);
  }

  get textContent() {
    let result = '';
    const _recurse = el => {
      if (el.nodeType === Node.TEXT_NODE) {
        result += el.value;
      } else if (el.childNodes) {
        for (let i = 0; i < el.childNodes.length; i++) {
          _recurse(el.childNodes[i]);
        }
      }
    };
    _recurse(this);
    return result;
  }
  set textContent(textContent) {
    textContent = textContent + '';

    while (this.childNodes.length > 0) {
      this.removeChild(this.childNodes[this.childNodes.length - 1]);
    }
    this.appendChild(this.ownerDocument.createTextNode(textContent));
  }

  get onclick() {
    return _elementGetter(this, 'click');
  }
  set onclick(onclick) {
    _elementSetter(this, 'click', onclick);
  }
  get onmousedown() {
    return _elementGetter(this, 'mousedown');
  }
  set onmousedown(onmousedown) {
    _elementSetter(this, 'mousedown', onmousedown);
  }
  get onmouseup() {
    return _elementGetter(this, 'mouseup');
  }
  set onmouseup(onmouseup) {
    _elementSetter(this, 'mouseup', onmouseup);
  }

  /**
   * Also the output when logging to console or debugger.
   */
  get outerHTML() {
    const _getIndent = depth => Array(depth*2 + 1).join(' ');
    const _recurse = (el, depth = 0) => {
      let result = '';
      if (el.tagName) {
        const tagName = el.tagName.toLowerCase();
        const indent = _getIndent(depth);
        const isAutoClosingTag = autoClosingTags[tagName];

        result += indent;
        result += '<' + tagName;
        for (let i = 0; i < el.attrs.length; i++) {
          const attr = el.attrs[i];
          result += ' ' + attr.name + '=' + JSON.stringify(attr.value);
        }
        if (isAutoClosingTag) {
          result += '/';
        }
        result += '>';

        if (!isAutoClosingTag) {
          let childrenResult = '';
          const childNodes = el.childNodes.concat(el.contentDocument ? [el.contentDocument] : []);
          for (let i = 0; i < childNodes.length; i++) {
            const childResult = _recurse(childNodes[i], depth + 1);
            if (childResult && !childrenResult) {
              childrenResult += '\n';
            }
            childrenResult += childResult;
          }
          if (childrenResult) {
            result += childrenResult;
            result += indent;
          }
          result += '</' + tagName + '>';
        }
        if (depth !== 0) {
          result += '\n';
        }
      } else if (el.constructor.name === 'Text' && /\S/.test(el.value)) {
        result += _getIndent(depth);
        result += el.value;
        if (depth !== 0) {
          result += '\n';
        }
      } else if (el.constructor.name === 'Comment') {
        result += _getIndent(depth);
        result += '<!--' + el.value + '-->';
        if (depth !== 0) {
          result += '\n';
        }
      }
      return result;
    };
    return _recurse(this);
  }

  requestPointerLock() {
    if (this.ownerDocument[symbols.pointerLockElementSymbol] === null) {
      this.ownerDocument[symbols.pointerLockElementSymbol] = this;

      process.nextTick(() => {
        this.ownerDocument._emit('pointerlockchange');
      });
    }
  }

  requestFullscreen() {
    /* if (this.ownerDocument[symbols.fullscreenElementSymbol] === null) { // XXX
      this.ownerDocument[symbols.fullscreenElementSymbol] = this;

      process.nextTick(() => {
        this.ownerDocument._emit('fullscreenchange');
      });
    } */
  }

  /**
   * For logging to console or debugger.
   */
  [util.inspect.custom]() {
    return this.outerHTML;
  }

  traverse(fn) {
    const _recurse = node => {
      const result = fn(node);
      if (result !== undefined) {
        return result;
      } else {
        if (node.childNodes) {
          for (let i = 0; i < node.childNodes.length; i++) {
            const result = _recurse(node.childNodes[i]);
            if (result !== undefined) {
              return result;
            }
          }
        }
        if (node.contentDocument) {
          return _recurse(node.contentDocument);
        }
      }
    };
    return _recurse(this);
  }
  async traverseAsync(fn) {
    const nodes = [];
    (function _recurse(node) {
      nodes.push(node);
      if (node.childNodes) {
        for (let i = 0; i < node.childNodes.length; i++) {
          _recurse(node.childNodes[i]);
        }
      }
      if (node.contentDocument) {
        _recurse(node.contentDocument);
      }
    })(this);

    for (let i = 0; i < nodes.length; i++) {
      const result = await fn(nodes[i]);
      if (result !== undefined) {
        return result;
      }
    }
  }
}
module.exports.Element = Element;

class HTMLElement extends Element {
  constructor(window, tagName = 'DIV', attrs = [], value = '', location = null) {
    if (HTMLElement.upgradeElement) {
      return HTMLElement.upgradeElement;
    }

    const extension = window.customElements.extensions[tagName];
    if (extension) {
      attrs.push({
        name: 'is',
        value: extension,
      });
    }

    super(window, tagName, attrs, value, location);

    this._style = null;
    this[symbols.computedStyleSymbol] = null;

    this.on('attribute', (name, value) => {
      if (name === 'class' && this._classList) {
        _resetClassList(this._classList, value);
      } else if (name === 'style') {
        if (this._style) {
          this._style.reset();
        }
        if (this[symbols.computedStyleSymbol]) {
          this[symbols.computedStyleSymbol] = null;
        }
      }
    });
  }

  get offsetWidth() {
    return this.clientWidth;
  }
  set offsetWidth(offsetWidth) {}
  get offsetHeight() {
    return this.clientHeight;
  }
  set offsetHeight(offsetHeight) {}
  get offsetTop() {
    return 0;
  }
  set offsetTop(offsetTop) {}
  get offsetLeft() {
    return 0;
  }
  set offsetLeft(offsetLeft) {}

  get offsetParent() {
    const body = this.ownerDocument.body;
    for (let el = this; el; el = el.parentNode) {
      if (el.parentNode === body) {
        return body;
      }
    }
    return null;
  }
  set offsetParent(offsetParent) {}

  get style() {
    if (!this._style) {
      this._style = _makeStyleProxy(this);
    }
    return this._style;
  }
  set style(style) {}
}
HTMLElement.upgradeElement = null;
module.exports.HTMLElement = HTMLElement;

function getAnchorUrl(anchorEl) {
  return new url.URL(anchorEl.href, anchorEl.ownerDocument.defaultView.location.toString());
}

class HTMLAnchorElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'A', attrs, value, location);
  }

  get href() {
    return this.getAttribute('href') || '';
  }
  set href(href) {
    href = href + '';
    this.setAttribute('href', href);
  }
  get hash() {
    return getAnchorUrl(this).hash || '';
  }
  set hash(hash) {
    hash = hash + '';
    const u = getAnchorUrl(this);
    u.hash = hash;
    this.href = u.href;
  }
  get host() {
    return getAnchorUrl(this).host || '';
  }
  set host(host) {
    host = host + '';
    const u = getAnchorUrl(this);
    u.host = host;
    this.href = u.href;
  }
  get hostname() {
    return getAnchorUrl(this).hostname || '';
  }
  set hostname(hostname) {
    hostname = hostname + '';
    const u = getAnchorUrl(this);
    u.hostname = hostname;
    this.href = u.href;
  }
  get password() {
    return getAnchorUrl(this).password || '';
  }
  set password(password) {
    password = password + '';
    const u = getAnchorUrl(this);
    u.password = password;
    this.href = u.href;
  }
  get origin() {
    return getAnchorUrl(this).origin || '';
  }
  set origin(origin) {
    origin = origin + '';
    const u = getAnchorUrl(this);
    u.origin = origin;
    this.href = u.href;
  }
  get pathname() {
    return getAnchorUrl(this).pathname || '';
  }
  set pathname(pathname) {
    pathname = pathname + '';
    const u = getAnchorUrl(this);
    u.pathname = pathname;
    this.href = u.href;
  }
  get port() {
    return getAnchorUrl(this).port || '';
  }
  set port(port) {
    port = port + '';
    const u = getAnchorUrl(this);
    u.port = port;
    this.href = u.href;
  }
  get protocol() {
    return getAnchorUrl(this).protocol || '';
  }
  set protocol(protocol) {
    protocol = protocol + '';
    const u = getAnchorUrl(this);
    u.protocol = protocol;
    this.href = u.href;
  }
  get search() {
    return getAnchorUrl(this).search || '';
  }
  set search(search) {
    search = search + '';
    const u = getAnchorUrl(this);
    u.search = search;
    this.href = u.href;
  }
  get username() {
    return getAnchorUrl(this).username || '';
  }
  set username(username) {
    username = username + '';
    const u = getAnchorUrl(this);
    u.username = username;
    this.href = u.href;
  }
}
module.exports.HTMLAnchorElement = HTMLAnchorElement;

class HTMLLoadableElement extends HTMLElement {
  constructor(window, tagName, attrs = [], value = '', location = null) {
    super(window, tagName, attrs, value, location);
  }

  get onload() {
    return _elementGetter(this, 'load');
  }
  set onload(onload) {
    _elementSetter(this, 'load', onload);
  }

  get onerror() {
    return _elementGetter(this, 'error');
  }
  set onerror(onerror) {
    _elementSetter(this, 'error', onerror);
  }
}
module.exports.HTMLLoadableElement = HTMLLoadableElement;

class HTMLHeadElement extends HTMLElement {
  constructor(window) {
    super(window, 'HEAD');
  }
}

class HTMLBodyElement extends HTMLElement {
  constructor(window) {
    super(window, 'BODY');
  }

  get clientWidth() {
    return this.ownerDocument.defaultView.innerWidth;
  }
  set clientWidth(clientWidth) {}
  get clientHeight() {
    return this.ownerDocument.defaultView.innerHeight;
  }
  set clientHeight(clientHeight) {}
}
module.exports.HTMLBodyElement = HTMLBodyElement;

class HTMLStyleElement extends HTMLLoadableElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'STYLE', attrs, value, location);

    this.stylesheet = null;

    this.on('innerHTML', innerHTML => {
      Promise.resolve()
        .then(() => css.parse(innerHTML).stylesheet)
        .then(stylesheet => {
          this.stylesheet = stylesheet;
          this.ownerDocument.defaultView[symbols.styleEpochSymbol]++;
          this.dispatchEvent(new Event('load', {target: this}));
        })
        .catch(err => {
          const e = new ErrorEvent('error', {target: this});
          e.message = err.message;
          e.stack = err.stack;
          this.dispatchEvent(e);
        });
    });
  }

  get src() {
    return this.getAttribute('src') || '';
  }
  set src(src) {
    src = src + '';
    this.setAttribute('src', src);
  }

  get type() {
    type = type + '';
    return this.getAttribute('type') || '';
  }
  set type(type) {
    this.setAttribute('type', type);
  }

  set innerHTML(innerHTML) {
    innerHTML = innerHTML + '';
    this._emit('innerHTML', innerHTML);
  }

  [symbols.runSymbol]() {
    let running = false;
    if (this.childNodes.length > 0) {
      this.innerHTML = this.childNodes[0].value;
      running = true;
    }
    return running ? _loadPromise(this) : Promise.resolve();
  }
}
module.exports.HTMLStyleElement = HTMLStyleElement;

class HTMLLinkElement extends HTMLLoadableElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'LINK', attrs, value, location);

    this.stylesheet = null;

    this.on('attribute', (name, value) => {
      if (this.isRunnable() && !this.readyState) {
        this.loadRunNow();
      }
    });
  }

  loadRunNow() {
    this.readyState = 'loading';

    const url = _mapUrl(this.href, this.ownerDocument.defaultView);

    return this.ownerDocument.resources.addResource((onprogress, cb) => {
      this.ownerDocument.defaultView.fetch(url)
        .then(res => {
          if (res.status >= 200 && res.status < 300) {
            return res.text();
          } else {
            return Promise.reject(new Error('link href got invalid status code: ' + res.status + ' : ' + url));
          }
        })
        .then(s => css.parse(s).stylesheet)
        .then(stylesheet => {
          this.stylesheet = stylesheet;
          this.ownerDocument.defaultView[symbols.styleEpochSymbol]++;

          this.readyState = 'complete';

          const e = new Event('load', {target: this});
          this._dispatchEventOnDocumentReady(e);

          cb();
        })
        .catch(err => {
          this.readyState = 'complete';

          const e = new ErrorEvent('error', {target: this});
          e.message = err.message;
          e.stack = err.stack;
          this._dispatchEventOnDocumentReady(e);

          cb(err);
        });
    });
  }

  get rel() {
    return this.getAttribute('rel') || '';
  }
  set rel(rel) {
    rel = rel + '';
    this.setAttribute('rel', rel);
  }

  get href() {
    return this.getAttribute('href') || '';
  }
  set href(href) {
    href = href + '';
    this.setAttribute('href', href);
  }

  get type() {
    type = type + '';
    return this.getAttribute('type') || '';
  }
  set type(type) {
    this.setAttribute('type', type);
  }

  isRunnable() {
    return this.rel === 'stylesheet' && !!this.href;
  }

  [symbols.runSymbol]() {
    if (this.isRunnable() && !this.readyState) {
      if (this.attributes.href) {
        return this.loadRunNow();
      }
    }
    return Promise.resolve();
  }
}
module.exports.HTMLLinkElement = HTMLLinkElement;

const _mapUrl = (u, window) => {
  const v = window[symbols.optionsSymbol].replacements[u];
  return v !== undefined ? v : u;
};
class HTMLScriptElement extends HTMLLoadableElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'SCRIPT', attrs, value, location);

    this.readyState = null;

    const _loadRun = async => {
      if (!async) {
        this.ownerDocument[symbols.addRunSymbol](this.loadRunNow.bind(this));
      } else {
        this.loadRunNow();
      }
    };
    this.on('attribute', (name, value) => {
      if (name === 'src' && value && this.isRunnable() && this.isConnected && !this.readyState) {
        const async = this.getAttribute('async');
        _loadRun(async !== null ? async !== 'false' : false);
      }
    });
    this.on('attached', () => {
      if (this.ownerDocument.readyState !== 'loading' && this.getAttribute('src') && this.isRunnable() && this.isConnected && !this.readyState) {
        const async = this.getAttribute('async');
        _loadRun(async !== null ? async !== 'false' : true);
      }
    });
    this.on('innerHTML', innerHTML => {
      if (this.isRunnable() && this.isConnected && !this.readyState) {
        this.loadRunNow();
      }
    });
  }

  get src() {
    const src = this.getAttribute('src');
    return src ? _normalizeUrl(src, this.ownerDocument.defaultView[symbols.optionsSymbol].baseUrl) : '';
  }
  set src(src) {
    src = src + '';
    this.setAttribute('src', src);
  }

  get type() {
    return this.getAttribute('type') || '';
  }
  set type(type) {
    type = type + '';
    this.setAttribute('type', type);
  }

  get async() {
    const async = this.getAttribute('async');
    return async === null || async !== 'false';
  }
  set async(async) {
    async = async + '';
    this.setAttribute('async', async);
  }

  get text() {
    let result = '';
    this.traverse(el => {
      if (el.nodeType === Node.TEXT_NODE) {
        result += el.value;
      }
    });
    return result;
  }
  set text(text) {
    this.textContent = text;
  }

  get innerHTML() {
    return this.textContent;
  }
  set innerHTML(innerHTML) {
    innerHTML = innerHTML + '';

    this.childNodes = new NodeList([this.ownerDocument.createTextNode(innerHTML)]);
    this._emit('innerHTML', innerHTML);
  }

  isRunnable() {
    return !this.type || /^(?:(?:text|application)\/javascript|application\/ecmascript|module)$/.test(this.type);
  }

  isModule() {
    return this.type === 'module';
  }

  loadRunNow() {
    this.readyState = 'loading';

    return this.ownerDocument.resources.addResource((onprogress, cb) => {
      const _getSrc = () => {
        const innerHTML = this.childNodes.length > 0 ? this.childNodes[0].value : '';
        if (innerHTML) {
          const url = this.ownerDocument.defaultView.location.href;
          const isModule = this.isModule();
          return Promise.resolve({
            s: innerHTML,
            url,
            isModule,
          });
        } else {
          const url = _mapUrl(this.src, this.ownerDocument.defaultView);
          const isModule = this.isModule();
          return _fetch(url)
            .then(s => ({
              s,
              url,
              isModule,
            }));
        }
      };
      const _fetch = async url => {
        const res = await this.ownerDocument.defaultView.fetch(url);
        if (res.status >= 200 && res.status < 300) {
          return await res.text();
        } else {
          throw new Error('script src got invalid status code: ' + res.status + ' : ' + url);
        }
      };

      return _getSrc()
        .then(async ({s, url, isModule}) => {
          const opts = {
            lineOffset : this.location && this.location.line !== null ? this.location.line - 1 : 0,
            columnOffset: this.location && this.location.col !== null ? this.location.col - 1 : 0,
          };

          if (isModule) {
            opts.url = url;
            const script = new vm.SourceTextModule(s, opts);
            await script.link(async (url, {url: baseUrl}) => {
              url = _mapUrl(_normalizeUrl(url, baseUrl), this.ownerDocument.defaultView);
              const s = await _fetch(url);
              return new vm.SourceTextModule(s, {
                url,
              });
            });
            script.instantiate();
            await script.evaluate();
          } else {
            opts.filename = url;
            vm.runInThisContext(s, opts);
          }
        })
        .then(() => {
          this.readyState = 'complete';

          this.dispatchEvent(new Event('load', {target: this}));

          cb();
        })
        .catch(err => {
          this.readyState = 'complete';

          const e = new ErrorEvent('error', {target: this});
          e.message = err.message;
          e.stack = err.stack;
          this.dispatchEvent(e);

          cb(err);
        });
    });
  }

  async [symbols.runSymbol]() {
    if (this.isRunnable() && !this.readyState) {
      await this.loadRunNow();
    }
  }
}
module.exports.HTMLScriptElement = HTMLScriptElement;

class HTMLSrcableElement extends HTMLLoadableElement {
  constructor(window, tagName = null, attrs = [], value = '', location = null) {
    super(window, tagName, attrs, value, location);

    this.readyState = null;
  }

  get src() {
    return this.getAttribute('src');
  }
  set src(value) {
    this.setAttribute('src', value);
  }

  [symbols.runSymbol]() {
    const srcAttr = this.attributes.src;
    if (srcAttr && !this.readyState) {
      this._emit('attribute', 'src', srcAttr.value);
    }
    return Promise.resolve();
  }
}
module.exports.HTMLSrcableElement = HTMLSrcableElement;

class HTMLMediaElement extends HTMLSrcableElement {
  constructor(window, tagName = null, attrs = [], value = '', location = null) {
    super(window, tagName, attrs, value, location);

    this._startTime = 0;
    this._startTimestamp = null;
  }

  play() {
    this._startTimestamp = Date.now();

    return Promise.resolve();
  }
  pause() {}
  load() {}

  get paused() {
    return true;
  }
  /* set paused(paused) {
    this._startTime = this.currentTime;
    this._startTimestamp = null;
  } */
  get currentTime() {
    return this._startTime + (this._startTimestamp !== null ? (Date.now() - this._startTimestamp) : 0);
  }
  set currentTime(currentTime) {}
  get duration() {
    return 1;
  }
  set duration(duration) {}
  get loop() {
    return false;
  }
  set loop(loop) {}
  get autoplay() {
    return false;
  }
  set autoplay(autoplay) {}

  canPlayType(type) {
    return ''; // XXX
  }

  get oncanplay() {
    return _elementGetter(this, 'canplay');
  }
  set oncanplay(oncanplay) {
    _elementSetter(this, 'canplay', oncanplay);
  }

  get oncanplaythrough() {
    return _elementGetter(this, 'canplaythrough');
  }
  set oncanplaythrough(oncanplaythrough) {
    _elementSetter(this, 'canplaythrough', oncanplaythrough);
  }

  get onerror() {
    return _elementGetter(this, 'error');
  }
  set onerror(onerror) {
    _elementSetter(this, 'error', onerror);
  }

  get onended() {
    return _elementGetter(this, 'ended');
  }
  set onended(onended) {
    _elementSetter(this, 'ended', onended);
  }

  get HAVE_NOTHING() {
    return HTMLMediaElement.HAVE_NOTHING;
  }
  set HAVE_NOTHING(v) {}
  get HAVE_METADATA() {
    return HTMLMediaElement.HAVE_METADATA;
  }
  set HAVE_METADATA(v) {}
  get HAVE_CURRENT_DATA() {
    return HTMLMediaElement.HAVE_CURRENT_DATA;
  }
  set HAVE_CURRENT_DATA(v) {}
  get HAVE_FUTURE_DATA() {
    return HTMLMediaElement.HAVE_FUTURE_DATA;
  }
  set HAVE_FUTURE_DATA(v) {}
  get HAVE_ENOUGH_DATA() {
    return HTMLMediaElement.HAVE_ENOUGH_DATA;
  }
  set HAVE_ENOUGH_DATA(v) {}
}
HTMLMediaElement.HAVE_NOTHING = 0;
HTMLMediaElement.HAVE_METADATA = 1;
HTMLMediaElement.HAVE_CURRENT_DATA = 2;
HTMLMediaElement.HAVE_FUTURE_DATA = 3;
HTMLMediaElement.HAVE_ENOUGH_DATA = 4;
module.exports.HTMLMediaElement = HTMLMediaElement;

class HTMLSourceElement extends HTMLSrcableElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'SOURCE', attrs, value, location);
  }
}
module.exports.HTMLSourceElement = HTMLSourceElement;

class SVGElement {}
module.exports.SVGElement = SVGElement;

const _parseVector = s => {
  if (Array.isArray(s)) {
    s = s.join(' ');
  }

  const result = [];
  const ss = s.split(' ');
  for (let i = 0; i < ss.length; i++) {
    const s = ss[i];
    const n = parseFloat(s);
    if (!isNaN(n)) {
      result.push(n);
    } else {
      return null;
    }
  }
  return result;
};
class HTMLIFrameElement extends HTMLSrcableElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'IFRAME', attrs, value, location);

    this.contentWindow = null;
    this.contentDocument = null;
    // this.live = true;
    this.epoch = 0;

    this.browser = null;
    this.onconsole = null;
    this.xrOffset = new XRRigidTransform();

    const _resetContentWindowDocument = () => {
      const contentDocument = {
        _emit() {},
        on() {},
        removeListener() {},
        open() {},
        write() {},
        close() {},
        documentElement: {
          getBoundingClientRect() {
            return new DOMRect(0, 0, 0, 0);
          },
        },
      };
      this.contentWindow = {
        document: contentDocument,
      };
      this.contentDocument = contentDocument;
    };

    this.on('attribute', (name, value) => {
      if (name === 'src' && value) {
        const localEpoch = ++this.epoch;

        this.readyState = 'loading';

        _resetContentWindowDocument();

        let url = value;
        const match = url.match(/^javascript:(.+)$/); // XXX should support this for regular fetches too
        if (match) {
          url = 'data:text/html,' + encodeURIComponent(`<!doctype html><html><head><script>${match[1]}</script></head></html>`);
        }
        const oldUrl = url;
        url = _normalizeUrl(url, this.ownerDocument.defaultView[symbols.optionsSymbol].baseUrl);

        this.ownerDocument.resources.addResource((onprogress, cb) => {
          (async () => {
            if (this.d === 2) {
              if (this.browser) {
                this.browser.destroy();
                this.browser = null;
              }

              const context = GlobalContext.contexts.find(context => ['WebGLRenderingContext', 'WebGL2RenderingContext'].includes(context.constructor.name) && context.canvas.ownerDocument === this.ownerDocument);
              if (context) {
                const browser = (() => {
                  const width = this.width || context.canvas.ownerDocument.defaultView.innerWidth;
                  const height = this.height || context.canvas.ownerDocument.defaultView.innerHeight;

                  if (bindings.nativePlatform === 'android') {
                    return new bindings.nativeBrowser.Browser(context, width, height, url);
                  } else {
                    return new ElectronVm({
                      url,
                      width,
                      height,
                      devicePixelRatio: context.canvas.ownerDocument.defaultView.devicePixelRatio,
                      inline: this.inline,
                      transparent: !this.inline,
                      context,
                    });
                  }
                })();
                this.browser = browser;

                let onmessage = null;
                const self = this;
                this.contentWindow = {
                  _emit() {},
                  document: this.contentDocument,
                  location: {
                    href: url,
                  },
                  postMessage: browser.postMessage && browser.postMessage.bind(browser), // XXX
                  get onmessage() {
                    return onmessage;
                  },
                  set onmessage(newOnmessage) {
                    onmessage = newOnmessage;
                  },
                  addEventListener() {
                    browser.on && browser.on.apply(browser, arguments); // XXX
                  },
                  removeEventListener() {
                    browser.removeListener && browser.removeListener.apply(browser, arguments); // XXX
                  },
                  destroy() {
                    self.browser.destroy();
                    self.browser = null;
                  },
                };

                const _load = () => {
                  this.readyState = 'complete';

                  this.dispatchEvent(new Event('load', {target: this}));

                  cb();
                };
                if (browser.on) {
                  browser.on('message', data => {
                    if (onmessage) {
                      const e = new MessageEvent('messaage', {
                        data,
                      });
                      onmessage(e);
                    }
                  });
                  browser.once('load', _load);
                } else {
                  process.nextTick(_load); // XXX make this an actual event
                }
              } else {
                throw new Error('iframe owner document does not have a WebGL context');
              }
            } else {
              const res = await this.ownerDocument.defaultView.fetch(url);
              if (this.epoch !== localEpoch) {
                return;
              }
              if (res.status >= 200 && res.status < 300) {
                const htmlString = await res.text();
                if (this.epoch !== localEpoch) {
                  return;
                }

                const parentWindow = this.ownerDocument.defaultView;
                const options = parentWindow[symbols.optionsSymbol];

                url = _normalizeUrl(res.url, options.baseUrl);
                const parent = {};
                const top = parentWindow === parentWindow.top ? parent : {};
                this.contentWindow = _makeWindow({
                  url,
                  baseUrl: url,
                  args: options.args,
                  dataPath: options.dataPath,
                  replacements: options.replacements,
                  parent,
                  top,
                  htmlString,
                  hidden: this.d === 3,
                  xrOffsetBuffer: this.xrOffset._buffer,
                  onnavigate: (href) => {
                    this.readyState = null;

                    this.setAttribute('src', href);
                  },
                  onrequest(req) {
                    parentPort.postMessage(req);
                  },
                  onxrmode(event) {
                    this.dispatchEvent(new CustomEvent('xrmode', {
                      detail: {
                        xr: event.xr,
                      },
                    }));
                  },
                  onhapticpulse(event) {
                    parentPort.postMessage({
                      method: 'emit',
                      type: 'hapticPulse',
                      event,
                    });
                  },
                  onpaymentrequest(event) {
                    if (window.listeners('paymentrequest').length > 0) {
                      window.dispatchEvent(new CustomEvent('paymentrequest', {
                        detail: event,
                      }));
                    } else {
                      parentPort.postMessage({
                        method: 'emit',
                        type: 'paymentRequest',
                        event,
                      });
                    }
                  },
                });
                this.contentWindow.document = this.contentDocument;

                this.readyState = 'complete';

                this.dispatchEvent(new Event('load', {target: this}));

                cb();
              } else {
                throw new Error('iframe src got invalid status code: ' + res.status + ' : ' + url);
              }
            }
          })()
            .catch(err => {
              console.error(err);

              this.readyState = 'complete';

              this.dispatchEvent(new Event('load', {target: this}));

              cb(err);
            })
        });
      } else if (name === 'position' || name === 'orientation' || name === 'scale') {
        const v = _parseVector(value);
        if (name === 'position' && v.length === 3) {
          this.xrOffset.position.set(v);
          this.xrOffset.pushUpdate();
        } else if (name === 'orientation' && v.length === 4) {
          this.xrOffset.orientation.set(v);
          this.xrOffset.pushUpdate();
        } else if (name === 'scale' && v.length === 3) {
          this.xrOffset.scale.set(v);
          this.xrOffset.pushUpdate();
        }
      } else if (name === 'width') {
        if (this.browser) {
          this.browser.width = this.width;
        }
      } else if (name === 'height') {
        if (this.browser) {
          this.browser.height = this.height;
        }
      } else if (name === 'devicePixelRatio') {
        if (this.browser) {
          this.browser.scale = this.devicePixelRatio;
        }
      } else if (name === 'inline') {
        if (this.browser) {
          this.browser.inline = this.inline;
        }
      }
    });
    this.on('attached', () => {
      if (this.ownerDocument.readyState !== 'loading' && !this.contentWindow) {
        _resetContentWindowDocument();
      }
    });
    /* this.on('destroy', () => {
      if (this.contentWindow) {
        this.contentWindow.destroy();
        this.contentWindow = null;
      }
      this.contentDocument = null;

      if (this.browser) {
        this.browser.destroy(); // XXX support this
      }
    }); */
  }

  get width() {
    return parseInt(this.getAttribute('width') || bindings.nativeWindow.getScreenSize()[0]/2 + '', 10);
  }
  set width(value) {
    if (typeof value === 'number' && isFinite(value)) {
      this.setAttribute('width', value);
    }
  }
  get height() {
    return parseInt(this.getAttribute('height') || bindings.nativeWindow.getScreenSize()[1]/2 + '', 10);
  }
  set height(value) {
    if (typeof value === 'number' && isFinite(value)) {
      this.setAttribute('height', value);
    }
  }
  get devicePixelRatio() {
    return parseInt(this.getAttribute('devicePixelRatio') || 1 + '', 10);
  }
  set devicePixelRatio(value) {
    if (typeof value === 'number' && isFinite(value)) {
      this.setAttribute('devicePixelRatio', value);
    }
  }

  get d() {
    const d = parseInt(this.getAttribute('d') + '', 10);
    return isFinite(d) ? d : null;
  }
  set d(value) {
    if (typeof value === 'number' && isFinite(value)) {
      this.setAttribute('d', value);
    }
  }
  get inline() {
    return this.getAttribute('inline') !== 'false';
  }
  set inline(inline) {
    if (inline) {
      this.removeAttribute('inline');
    } else {
      this.setAttribute('inline', 'false');
    }
  }

  setPosition(x, y) {
    this.browser && this.browser.setPosition && this.browser.setPosition(x, y);
  }
  setSize(width, height) {
    this.browser && this.browser.setSize && this.browser.setSize(width, height);
  }
  show() {
    this.browser && this.browser.show && this.browser.show();
  }
  hide() {
    this.browser && this.browser.hide && this.browser.hide();
  }
  setAlwaysOnTop(value) {
    this.browser && this.browser.setAlwaysOnTop && this.browser.setAlwaysOnTop(value);
  }

  get texture() {
    if (this.d === 2) {
      return this.browser ? this.browser.texture : null;
    } else {
      return null;
    }
  }
  set texture(texture) {}

  get position() {
    return this.getAttribute('position');
  }
  set position(position) {
    if (Array.isArray(position)) {
      position = position.join(' ');
    }
    this.setAttribute('position', position);
  }

  get orientation() {
    return this.getAttribute('orientation');
  }
  set orientation(orientation) {
    if (Array.isArray(orientation)) {
      orientation = orientation.join(' ');
    }
    this.setAttribute('orientation', orientation);
  }

  get scale() {
    return this.getAttribute('scale');
  }
  set scale(scale) {
    if (Array.isArray(scale)) {
      scale = scale.join(' ');
    }
    this.setAttribute('scale', scale);
  }

  back() { // XXX should use native navigation APIs for these
    this.browser && this.browser.back();
  }
  forward() {
    this.browser && this.browser.forward();
  }
  reload() {
    this.browser && this.browser.reload();
  }

  sendMouseMove(x, y) {
    this.browser && this.browser.sendMouseMove(x, y);
  }
  sendMouseDown(x, y, button) {
    this.browser && this.browser.sendMouseDown(x, y, button);
  }
  sendClick(x, y, button) {
    this.browser && this.browser.sendClick(x, y, button);
  }
  sendMouseUp(x, y, button) {
    this.browser && this.browser.sendMouseUp(x, y, button);
  }
  sendMouseWheel(x, y, deltaX, deltaY) {
    this.browser && this.browser.sendMouseWheel(x, y, deltaX, deltaY);
  }
  sendKeyDown(key, modifiers) {
    if (this.browser) {
      this.browser.sendKeyDown(key, modifiers);
      if (key === 13) {
        this.browser.sendKeyPress(key, modifiers);
      }
    }
  }
  sendKeyUp(key, modifiers) {
    this.browser && this.browser.sendKeyUp(key, modifiers);
  }
  sendKeyPress(key, modifiers) {
    this.browser && this.browser.sendKeyPress(key, modifiers);
  }

  runJs(jsString = '', scriptUrl = '<unknown>', startLine = 1) {
    this.browser && this.browser.runJs(jsString, scriptUrl, startLine);
  }
  
  destroy() {
    if (this.contentWindow) {
      this.contentWindow.destroy();
    }

    /* if (this.live) {
      this._emit('destroy');
      this.live = false;
    } */
  }
}
module.exports.HTMLIFrameElement = HTMLIFrameElement;

class HTMLCanvasElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'CANVAS', attrs, value, location);

    this._context = null;

    this.on('attribute', (name, value) => {
      if (name === 'width' || name === 'height') {
        if (this._context && this._context.resize) {
          this._context.resize(this.width, this.height);
        }
      }
    });
  }

  get width() {
    return parseInt(this.getAttribute('width') || bindings.nativeWindow.getScreenSize()[0]/2 + '', 10);
  }
  set width(value) {
    if (typeof value === 'number' && isFinite(value)) {
      this.setAttribute('width', value);
    }
  }
  get height() {
    return parseInt(this.getAttribute('height') || bindings.nativeWindow.getScreenSize()[1]/2 + '', 10);
  }
  set height(value) {
    if (typeof value === 'number' && isFinite(value)) {
      this.setAttribute('height', value);
    }
  }
  setPosition(x, y) {
    if (this._context && (this._context.constructor.name === 'WebGLRenderingContext' || this._context.constructor.name === 'WebGL2RenderingContext')) {
      const windowHandle = this._context.getWindowHandle();
      bindings.nativeWindow.setWindowPos(windowHandle, x, y);
    }
  }
  setSize(width, height) {
    if (this._context && (this._context.constructor.name === 'WebGLRenderingContext' || this._context.constructor.name === 'WebGL2RenderingContext')) {
      const windowHandle = this._context.getWindowHandle();
      bindings.nativeWindow.setWindowSize(windowHandle, width, height);
    }
  }
  setFocus() {
    if (this._context && (this._context.constructor.name === 'WebGLRenderingContext' || this._context.constructor.name === 'WebGL2RenderingContext')) {
      const windowHandle = this._context.getWindowHandle();
      bindings.nativeWindow.setWindowFocus(windowHandle);
    }
  }

  get clientWidth() {
    return this.width;
  }
  set clientWidth(clientWidth) {}
  get clientHeight() {
    return this.height;
  }
  set clientHeight(clientHeight) {}

  getBoundingClientRect() {
    return new DOMRect(0, 0, this.clientWidth, this.clientHeight);
  }

  get data() {
    return (this._context && this._context.data) || null;
  }
  set data(data) {}

  get texture() {
    return (this._context && this._context.texture) || null;
  }
  set texture(texture) {}

  getContext(contextType, attrs = {}) {
    if (contextType === '2d') {
      if (this._context) {
        const windowHandle = this._context.getWindowHandle();
        const window = this.ownerDocument.defaultView;
        const canvas2dWindowHandle = window[symbols.canvas2dWindowHandle];
        if (!_windowHandleEquals(windowHandle, canvas2dWindowHandle)) {
          this._context.destroy();
        }
        this._context = null;
      }

      this._context = new GlobalContext.CanvasRenderingContext2D(this);
    } else if (contextType === 'webgl' || contextType === 'experimental-webgl' || contextType === 'webgl2' || contextType === 'xrpresent') {
      if (this._context) {
        this._context.destroy();
        this._context = null;
      }

      const window = this.ownerDocument.defaultView;

      if (!window[symbols.optionsSymbol].args || window[symbols.optionsSymbol].args.webgl === '1') {
        if (contextType === 'webgl' || contextType === 'experimental-webgl' || contextType === 'xrpresent') {
          this._context = new GlobalContext.WebGLRenderingContext(this, attrs);
        }
      } else {
        if (contextType === 'webgl' || contextType === 'experimental-webgl') {
          this._context = new GlobalContext.WebGLRenderingContext(this, attrs);
        } else {
          this._context = new GlobalContext.WebGL2RenderingContext(this, attrs);
        }
      }
    } else {
      if (this._context) {
        this._context.destroy();
        this._context = null;
      }
    }

    // hack: assume that getting a context means we might want to enter XR
    setTimeout(() => {
      window.vrdisplayactivate();
    });

    return this._context;
  }

  toDataURL(type, encoderOptions) {
    const arrayBuffer = this.toArrayBuffer(type, encoderOptions);
    return `data:${arrayBuffer.type};base64,${new Buffer(arrayBuffer).toString('base64')}`;
  }

  toBlob(cb, type, encoderOptions) {
    process.nextTick(() => {
      const arrayBuffer = this.toArrayBuffer(type, encoderOptions);
      const blob = new Blob();
      blob.buffer = new Buffer(arrayBuffer);
      cb(blob);
    });
  }

  toArrayBuffer(cb, type, encoderOptions) {
    if (!this._context) {
      this.getContext('2d');
    }
    return this._context.toArrayBuffer(type, encoderOptions);
  }

  captureStream(frameRate) {
    return {}; // XXX
  }
}
module.exports.HTMLCanvasElement = HTMLCanvasElement;

class HTMLTemplateElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'TEMPLATE', attrs, value, location);

    this._childNodes = new NodeList();
  }

  get content() {
    const window = this.ownerDocument.defaultView;
    const content = new window.DocumentFragment();
    content.ownerDocument = this.ownerDocument;
    content.childNodes = new NodeList(this._childNodes);
    return content;
  }
  set content(content) {}

  get childNodes() {
    return new NodeList();
  }
  set childNodes(childNodes) {
    this._childNodes = childNodes;
  }

  get children() {
    return [];
  }
  set children(children) {}

  get innerHTML() {
    return parse5.serialize({
      tagName: this.tagName,
      attrs: this.attrs,
      value: this.value,
      childNodes: this._childNodes,
    });
  }
  set innerHTML(innerHTML) {
    super.innerHTML = innerHTML;
  }
}
module.exports.HTMLTemplateElement = HTMLTemplateElement;

class HTMLTextareaElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'TEXTAREA', attrs, value, location);
  }

  get value() {
    return this.textContent;
  }
  set value(value) {
    if (this.ownerDocument) { // if this isn't initialization
      this.textContent = value;
    }
  }

  get textLength() {
    return this.value.length;
  }
  set textLength(textLength) {}

  get innerHTML() {
    return this.textContent;
  }
  set innerHTML(innerHTML) {
    this.textContent = htmlUnescape(innerHTML);
  }
}
module.exports.HTMLTextareaElement = HTMLTextareaElement;

class CharacterNode extends Node {
  constructor(window, value) {
    super(window);

    this.value = value;
  }

  get textContent() {
    return this.value;
  }
  set textContent(textContent) {
    this.value = textContent;

    this._emit('value');
  }

  get data() {
    return this.value;
  }
  set data(data) {
    this.value = data;

    this._emit('value');
  }
  get length() {
    return this.value.length;
  }
  set length(length) {}

  get firstChild() {
    return null;
  }
  set firstChild(firstChild) {}
  get lastChild() {
    return null;
  }
  set lastChild(lastChild) {}

  traverse(fn) {
    fn(this);
  }
}
module.exports.CharacterNode = CharacterNode;

class Text extends CharacterNode {
  constructor(window, value) {
    super(window, value);
  }

  get nodeType() {
    return Node.TEXT_NODE;
  }
  set nodeType(nodeType) {}

  get nodeName() {
    return '#text';
  }
  set nodeName(nodeName) {}

  get nodeValue() {
    return this.value;
  }
  set nodeValue(nodeValue) {
    this.value = nodeValue;
  }

  get firstChild() {
    return null;
  }
  set firstChild(firstChild) {}
  get lastChild() {
    return null;
  }
  set lastChild(lastChild) {}

  [util.inspect.custom]() {
    return JSON.stringify(this.value);
  }
}
module.exports.Text = Text;

class Comment extends CharacterNode {
  constructor(window, value) {
    super(window, value);
  }

  get nodeType() {
    return Node.COMMENT_NODE;
  }
  set nodeType(nodeType) {}

  get nodeName() {
    return '#comment';
  }
  set nodeName(nodeName) {}

  get nodeValue() {
    return this.value;
  }
  set nodeValue(nodeValue) {
    this.value = nodeValue;
  }

  [util.inspect.custom]() {
    return `<!--${this.value}-->`;
  }
}
module.exports.Comment = Comment;

class HTMLImageElement extends HTMLSrcableElement {
  constructor(window, attrs = [], value = '', location = null) {
    if (typeof attrs === 'number') {
      const width = attrs;
      const height = value;
      return new HTMLImageElement(window, [
        {
          name: 'width',
          value: width + '',
        },
        {
          name: 'height',
          value: height + '',
        },
      ], '', null);
    } else {
      super(window, 'IMG', attrs, value, location);

      this.image = new bindings.nativeImage();

      this.on('attribute', (name, value) => {
        if (name === 'src' && value) {
          this.readyState = 'loading';

          const src = value;

          this.ownerDocument.resources.addResource((onprogress, cb) => {
            this.ownerDocument.defaultView.fetch(src)
              .then(res => {
                if (res.status >= 200 && res.status < 300) {
                  return res.arrayBuffer();
                } else {
                  return Promise.reject(new Error(`img src got invalid status code (url: ${JSON.stringify(src)}, code: ${res.status})`));
                }
              })
              .then(arrayBuffer => new Promise((accept, reject) => {
                this.image.load(arrayBuffer, err => {
                  if (!err) {
                    accept();
                  } else {
                    reject(new Error(`failed to decode image: ${err.message} (url: ${JSON.stringify(src)}, size: ${arrayBuffer.byteLength}, message: ${err})`));
                  }
                });
              }))
              .then(() => {
                this.readyState = 'complete';

                this._dispatchEventOnDocumentReady(new Event('load', {target: this}));

                cb();
              })
              .catch(err => {
                console.warn('failed to load image:', src);

                this.readyState = 'complete';

                const e = new ErrorEvent('error', {target: this});
                e.message = err.message;
                e.stack = err.stack;
                this._dispatchEventOnDocumentReady(e);

                cb(err);
              });
          });
        }
      });
    }
  }

  get src() {
    return this.getAttribute('src');
  }
  set src(src) {
    this.setAttribute('src', src);
  }

  get onload() {
    return _elementGetter(this, 'load');
  }
  set onload(onload) {
    _elementSetter(this, 'load', onload);
  }

  get onerror() {
    return _elementGetter(this, 'error');
  }
  set onerror(onerror) {
    _elementSetter(this, 'error', onerror);
  }

  get width() {
    return this.image.width;
  }
  set width(width) {}
  get height() {
    return this.image.height;
  }
  set height(height) {}

  get naturalWidth() {
    return this.width;
  }
  set naturalWidth(naturalWidth) {}
  get naturalHeight() {
    return this.height;
  }
  set naturalHeight(naturalHeight) {}

  getBoundingClientRect() {
    return new DOMRect(0, 0, this.width, this.height);
  }

  get data() {
    return this.image.data;
  }
  set data(data) {}
};
module.exports.HTMLImageElement = HTMLImageElement;

class TimeRanges {
  constructor(ranges) {
    this._ranges = ranges;
  }

  start(i) {
    return this._ranges[i][0];
  }

  end(i) {
    return this._ranges[i][1];
  }

  get length() {
    return this._ranges.length;
  }
  set length(length) {}
}
module.exports.TimeRanges = TimeRanges;

class HTMLAudioElement extends HTMLMediaElement {
  constructor(window, attrs = [], value = '') {
    if (typeof attrs === 'string') {
      const src = attrs;
      const audio = new HTMLAudioElement(window, [], '', null);
      audio.src = src + '';
      return audio;
    } else {
      super(window, 'AUDIO', attrs, value);

      this.readyState = HTMLMediaElement.HAVE_NOTHING;
      this.audio = new bindings.nativeAudio.Audio();

      this.on('attribute', (name, value) => {
        if (name === 'src' && value) {
          const src = value;

          this.ownerDocument.resources.addResource((onprogress, cb) => {
            this.ownerDocument.defaultView.fetch(src)
              .then(res => {
                if (res.status >= 200 && res.status < 300) {
                  return res.arrayBuffer();
                } else {
                  return Promise.reject(new Error(`audio src got invalid status code (url: ${JSON.stringify(src)}, code: ${res.status})`));
                }
              })
              .then(arrayBuffer => new Promise((accept, reject) => {
                this.audio.load(arrayBuffer, err => {
                  if (!err) {
                    accept();
                  } else {
                    reject(new Error(`failed to decode audio: ${err.message} (url: ${JSON.stringify(src)}, size: ${arrayBuffer.byteLength})`));
                  }
                });
              }))
              .then(() => {
                this.readyState = HTMLMediaElement.HAVE_ENOUGH_DATA;

                const progressEvent = new Event('progress', {target: this});
                progressEvent.loaded = 1;
                progressEvent.total = 1;
                progressEvent.lengthComputable = true;
                this._emit(progressEvent);

                this._dispatchEventOnDocumentReady(new Event('loadeddata', {target: this}));
                this._dispatchEventOnDocumentReady(new Event('loadedmetadata', {target: this}));
                this._dispatchEventOnDocumentReady(new Event('canplay', {target: this}));
                this._dispatchEventOnDocumentReady(new Event('canplaythrough', {target: this}));

                if (this.autoplay) {
                  this.play();
                }

                cb();
              })
              .catch(err => {
                console.warn('failed to load audio:', src);

                const e = new ErrorEvent('error', {target: this});
                e.message = err.message;
                e.stack = err.stack;
                this._dispatchEventOnDocumentReady(e);

                cb(err);
              });
          });
        }
      });
    }
  }

  play() {
    this.audio.play();

    return Promise.resolve();
  }
  pause() {
    this.audio.pause();
  }

  get paused() {
    return this.audio ? this.audio.paused : true;
  }

  get currentTime() {
    return this.audio && this.audio.currentTime;
  }
  set currentTime(currentTime) {
    if (this.audio) {
      this.audio.currentTime = currentTime;
    }
  }

  get loop() {
    return this.audio ? this.audio.loop : false;
  }

  set loop(loop) {
    if (this.audio) {
      this.audio.loop = loop;
    }
  }

  get duration() {
    return this.audio && this.audio.duration;
  }
  set duration(duration) {
    if (this.audio) {
      this.audio.duration = duration;
    }
  }

  get autoplay() {
    const autoplay = this.getAttribute('autoplay');
    return !!autoplay || autoplay === '';
  }
  set autoplay(autoplay) {
    this.setAttribute('autoplay', autoplay);
  }

  get buffered() {
    return new TimeRanges([0, this.duration]);
  }
  set buffered(buffered) {}

  get onended() {
    return this.audio && this.audio.onended;
  }
  set onended(onended) {
    if (this.audio) {
      this.audio.onended = onended;
    }
  }

};
module.exports.HTMLAudioElement = HTMLAudioElement;

class HTMLVideoElement extends HTMLMediaElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'VIDEO', attrs, value, location);

    this.readyState = HTMLMediaElement.HAVE_NOTHING;
    this.data = new Uint8Array(0);

    this.on('attribute', (name, value) => {
      if (name === 'src' && value) {
        this.readyState = 'loading';

        const src = value;

        this.readyState = HTMLMediaElement.HAVE_ENOUGH_DATA;

        /* if (urls.has(value)) {
          const blob = urls.get(value);
          if (blob instanceof bindings.nativeVideo.VideoDevice) {
            this.video = blob;
          }
        } */

        this.ownerDocument.resources.addResource((onprogress, cb) => {
          const progressEvent = new Event('progress', {target: this});
          progressEvent.loaded = 1;
          progressEvent.total = 1;
          progressEvent.lengthComputable = true;
          this._emit(progressEvent);

          this.readyState = 'complete';

          this._dispatchEventOnDocumentReady(new Event('loadeddata', {target: this}));
          this._dispatchEventOnDocumentReady(new Event('loadedmetadata', {target: this}));
          this._dispatchEventOnDocumentReady(new Event('canplay', {target: this}));
          this._dispatchEventOnDocumentReady(new Event('canplaythrough', {target: this}));

          cb();
        });
      }
    });
  }

  get width() {
    return this.video ? this.video.width : 0;
  }
  set width(width) {}
  get height() {
    return this.video ? this.video.height : 0;
  }
  set height(height) {}

  get autoplay() {
    const autoplay = this.getAttribute('autoplay');
    return !!autoplay || autoplay === '';
  }
  set autoplay(autoplay) {
    this.setAttribute('autoplay', autoplay);
  }

  getBoundingClientRect() {
    return new DOMRect(0, 0, this.width, this.height);
  }

  get data() {
    return this.video ? this.video.data : null;
  }
  set data(data) {}

  play() {
    const _getDevice = facingMode => {
      switch (facingMode) {
        case 'user': return devices[0];
        case 'environment': return devices[1];
        case 'left': return devices[2];
        case 'right': return devices[3];
        default: return devices[0];
      }
    }
    const _getName = facingMode => (process.platform === 'darwin' ? '' : 'video=') + _getDevice(facingMode).name;
    const _getOptions = facingMode => {
      if (process.platform === 'darwin') {
        return 'framerate='+_getDevice(facingMode).modes[0].fps;
      } else {
        return null;
      }
    }
    if (this.video) {
      this.video.close();
      this.video.open(
        _getName(this.video.constraints.facingMode),
        _getOptions(this.video.constraints.facingMode)
      );
    }

    return Promise.resolve();
  }
  pause() {
    if (this.video) {
      this.video.close();
    }
  }

  get buffered() {
    return new TimeRanges([0, this.duration]);
  }
  set buffered(buffered) {}

  update() {
    if (this.video) {
      this.video.update();
    }
  }
}
module.exports.HTMLVideoElement = HTMLVideoElement;

/*
class HTMLVideoElement extends HTMLMediaElement {
  constructor(attrs = [], value = '') {
    super('VIDEO', attrs, value);

    this.readyState = HTMLMediaElement.HAVE_NOTHING;
    this.video = new bindings.nativeVideo.Video();

    this.on('attribute', (name, value) => {
      if (name === 'src' && value) {
        console.log('video downloading...');
        const src = value;

        this.ownerDocument.defaultView.fetch(src)
          .then(res => {
            console.log('video download res');
            if (res.status >= 200 && res.status < 300) {
              return res.arrayBuffer();
            } else {
              return Promise.reject(new Error(`video src got invalid status code (url: ${JSON.stringify(src)}, code: ${res.status})`));
            }
          })
          .then(arrayBuffer => {
            console.log('video download arraybuffer');
            try {
              this.video.load(arrayBuffer);
            } catch(err) {
              throw new Error(`failed to decode video: ${err.message} (url: ${JSON.stringify(src)}, size: ${arrayBuffer.byteLength})`);
            }
          })
          .then(() => {
            console.log('video download done');
            this.readyState = HTMLMediaElement.HAVE_ENOUGH_DATA;
            this._emit('canplay');
            this._emit('canplaythrough');
          })
          .catch(err => {
            this._emit('error', err);
          });
      } else if (name === 'loop') {
        this.video.loop = !!value || value === '';
      } else if (name === 'autoplay') {
        const autoplay = !!value || value === '';
        if (autoplay) {
          console.log('video set autoplay');
          const canplay = () => {
            console.log('video autoplay play');
            this.video.play();
            _cleanup();
          };
          const error = () => {
            _cleanup();
          };
          const _cleanup = () => {
            this.removeListener('canplay', canplay);
            this.removeListener('error', error);
          };
          this.on('canplay', canplay);
          this.on('error', error);
        }
      }
    });
  }

  get width() {
    return this.video.width;
  }
  set width(width) {}
  get height() {
    return this.video.height;
  }
  set height(height) {}

  get loop() {
    return this.getAttribute('loop');
  }
  set loop(loop) {
    this.setAttribute('loop', loop);
  }

  get autoplay() {
    return this.getAttribute('autoplay');
  }
  set autoplay(autoplay) {
    this.setAttribute('autoplay', autoplay);
  }

  getBoundingClientRect() {
    return new DOMRect(0, 0, this.width, this.height);
  }

  get data() {
    return this.video.data;
  }
  set data(data) {}

  play() {
    this.video.play();

    return Promise.resolve();
  }
  pause() {
    this.video.pause();
  }

  get currentTime() {
    return this.video && this.video.currentTime;
  }
  set currentTime(currentTime) {
    if (this.video) {
      this.video.currentTime = currentTime;
    }
  }

  get duration() {
    return this.video && this.video.duration;
  }
  set duration(duration) {
    if (this.video) {
      this.video.duration = duration;
    }
  }

  run() {
    let running = false;

    let sources;
    const srcAttr = this.attributes.src;
    if (srcAttr) {
      this._emit('attribute', 'src', srcAttr.value);
      running = true;
    } else if (sources = this.childNodes.filter(childNode => childNode.nodeType === Node.ELEMENT_NODE && childNode.matches('source'))) {
      for (let i = 0; i < sources.length; i++) {
        const source = sources[i];
        const {src} = source;
        if (src) {
          this.src = src;
          running = true;
          break;
        }
      }
    }
    const loopAttr = this.attributes.loop;
    const loopAttrValue = loopAttr && loopAttr.value;
    if (loopAttrValue || loopAttrValue === '') {
      this.loop = loopAttrValue;
    }
    const autoplayAttr = this.attributes.loop;
    const autoplayAttrValue = autoplayAttr && autoplayAttr.value;
    if (autoplayAttrValue || autoplayAttrValue === '') {
      this.autoplay = autoplayAttrValue;
    }

    return running;
  }
}
*/

function createImageBitmap(src, x, y, w, h, options) {
  let image;
  if (src.constructor.name === 'HTMLImageElement') {
    image = src.image;
  } else if (src.constructor.name === 'Blob') {
    image = new bindings.nativeImage();
    try {
      image.load(src.buffer);
    } catch (err) {
      return Promise.reject(new Error('failed to load image'));
    }
  } else {
    return Promise.reject(new Error('invalid arguments. Unknown constructor type: ' + src.constructor.name));
  }

  if (typeof x === 'object') {
    options = x;
    x = undefined;
  }

  x = x || 0;
  y = y || 0;
  w = w || image.width;
  h = h || image.height;
  const flipY = !!options && options.imageOrientation === 'flipY';
  const imageBitmap = new ImageBitmap(
    image,
    x,
    y,
    w,
    h,
    flipY,
  );
  return Promise.resolve(imageBitmap);
}
module.exports.createImageBitmap = createImageBitmap;

class HTMLDivElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'DIV', attrs, value, location);
  }
}
module.exports.HTMLDivElement = HTMLDivElement;

class HTMLUListElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'ULIST', attrs, value, location);
  }
}
module.exports.HTMLUListElement = HTMLUListElement;

class HTMLLIElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'LI', attrs, value, location);
  }
}
module.exports.HTMLLIElement = HTMLLIElement;

class HTMLTableElement extends HTMLElement {
  constructor(window, attrs = [], value = '', location = null) {
    super(window, 'TABLE', attrs, value, location);
  }
}
module.exports.HTMLTableElement = HTMLTableElement;

function _hash(s) {
  let result = 0;
  for (let i = 0; i < s.length; i++) {
    result += s.codePointAt(i);
  }
  return result;
}

const getBoundDOMElements = window => {
  const bind = (OldClass, makeClass) => {
    const NewClass = makeClass((a, b, c, d) => new OldClass(window, a, b, c, d));
    NewClass.prototype = OldClass.prototype;
    NewClass.constructor = OldClass;
    return NewClass;
  };
  return {
    Element: bind(Element, b => function Element() { return b.apply(this, arguments); }),
    HTMLElement: bind(HTMLElement, b => function HTMLElement() { return b.apply(this, arguments); }),
    HTMLHeadElement: bind(HTMLHeadElement, b => function HTMLHeadElement() { return b.apply(this, arguments); }),
    HTMLBodyElement: bind(HTMLBodyElement, b => function HTMLBodyElement() { return b.apply(this, arguments); }),
    HTMLAnchorElement: bind(HTMLAnchorElement, b => function HTMLAnchorElement() { return b.apply(this, arguments); }),
    HTMLStyleElement: bind(HTMLStyleElement, b => function HTMLStyleElement() { return b.apply(this, arguments); }),
    HTMLLinkElement: bind(HTMLLinkElement, b => function HTMLLinkElement() { return b.apply(this, arguments); }),
    HTMLScriptElement: bind(HTMLScriptElement, b => function HTMLScriptElement() { return b.apply(this, arguments); }),
    HTMLImageElement: bind(HTMLImageElement, b => function HTMLImageElement() { return b.apply(this, arguments); }),
    HTMLAudioElement: bind(HTMLAudioElement, b => function HTMLAudioElement() { return b.apply(this, arguments); }),
    HTMLVideoElement: bind(HTMLVideoElement, b => function HTMLVideoElement() { return b.apply(this, arguments); }),
    HTMLSourceElement: bind(HTMLSourceElement, b => function HTMLSourceElement() { return b.apply(this, arguments); }),
    SVGElement: bind(SVGElement, b => function SVGElement() { return b.apply(this, arguments); }),
    HTMLIFrameElement: bind(HTMLIFrameElement, b => function HTMLIFrameElement() { return b.apply(this, arguments); }),
    HTMLCanvasElement: bind(HTMLCanvasElement, b => function HTMLCanvasElement() { return b.apply(this, arguments); }),
    HTMLTextareaElement: bind(HTMLTextareaElement, b => function HTMLTextareaElement() { return b.apply(this, arguments); }),
    HTMLTemplateElement: bind(HTMLTemplateElement, b => function HTMLTemplateElement() { return b.apply(this, arguments); }),
    HTMLDivElement: bind(HTMLDivElement, b => function HTMLDivElement() { return b.apply(this, arguments); }),
    HTMLUListElement: bind(HTMLUListElement, b => function HTMLUListElement() { return b.apply(this, arguments); }),
    HTMLLIElement: bind(HTMLLIElement, b => function HTMLLIElement() { return b.apply(this, arguments); }),
    HTMLTableElement: bind(HTMLTableElement, b => function HTMLTableElement() { return b.apply(this, arguments); }),
    Node: bind(Node, b => function Node() { return b.apply(this, arguments); }),
    Text: bind(Text, b => function Text() { return b.apply(this, arguments); }),
    Comment: bind(Comment, b => function Comment() { return b.apply(this, arguments); }),
  };
};
module.exports.getBoundDOMElements = getBoundDOMElements;
