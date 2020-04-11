const {process} = global;

const parse5 = require('parse5');

const DOM = require('./DOM');
const {Event, EventTarget} = require('./Event');
const GlobalContext = require('./GlobalContext');
const nativeBindings = require('./native-bindings');
const symbols = require('./symbols');
const utils = require('./utils');

const _parseDocument = (s, window) => {
  const ast = parse5.parse(s, {
    sourceCodeLocationInfo: true,
  });
  ast.tagName = 'document';
  return _parseDocumentAst(ast, window, true);
};
module.exports._parseDocument = _parseDocument;
GlobalContext._parseDocument = _parseDocument;

const _parseDocumentAst = (ast, window, uppercase) => {
  const document = _fromAST(ast, window, null, null, uppercase);
  return initDocument(document, window);
};
module.exports._parseDocumentAst = _parseDocumentAst;

/**
 * Initialize document instance with properties and methods.
 */
function initDocument (document, window) {
  const html = document.childNodes.find(el => el.tagName === 'HTML');
  const documentElement = html || (document.childNodes.length > 0 ? document.childNodes[0] : null);
  const head = html ? html.childNodes.find(el => el.tagName === 'HEAD') : new window.HTMLHeadElement();
  const body = html ? html.childNodes.find(el => el.tagName === 'BODY') : new window.HTMLBodyElement();

  document.documentElement = documentElement;
  document.readyState = 'loading';
  document.head = head;
  document.body = body;
  document.location = window.location;
  document.URL = window.location.href;
  document.cookie = '';
  document.referrer = '';
  document.createElement = (tagName, options = {}) => {
    tagName = tagName.toUpperCase();
    const HTMLElementTemplate = window[symbols.htmlTagsSymbol][tagName];
    const element = HTMLElementTemplate ? new HTMLElementTemplate() : new window.HTMLElement(tagName);
    options.is && element.setAttribute('is', options.is);
    return element;
  };
  document.createElementNS = (namespace, tagName, options) => document.createElement(tagName, options);
  document.createDocumentFragment = () => new window.DocumentFragment();
  document.createTextNode = text => new window.Text(text);
  document.createComment = comment => new window.Comment(comment);
  document.createEvent = type => {
    switch (type) {
      case 'KeyboardEvent':
      case 'KeyboardEvents':
        return new KeyboardEvent();
      case 'MouseEvent':
      case 'MouseEvents':
        return new MouseEvent();
      case 'Event':
      case 'Events':
      case 'HTMLEvents':
        return new Event();
      default:
        throw new Error('invalid createEvent type: ' + type);
    }
  };
  document.createRange = () => new Range(window);
  document.importNode = (el, deep) => el.cloneNode(deep);
  document.scripts = utils._makeHtmlCollectionProxy(document.documentElement, 'script');
  document.styleSheets = [];
  document.implementation = new DOMImplementation(window);
  document.resources = new GlobalContext.Resources(); // non-standard
  document.activeElement = body;
  document.open = () => {
    document.innerHTML = '';
  };
  document.close = () => {};
  document.write = htmlString => {
    const childNodes = parse5.parseFragment(htmlString, {
      locationInfo: true,
    }).childNodes.map(childNode => _fromAST(childNode, window, document.body, document, true));
    for (let i = 0; i < childNodes.length; i++) {
      document.body.appendChild(childNodes[i]);
    }
  };
  document.execCommand = command => {
    if (command === 'copy') {
      // nothing
    } else if (command === 'paste') {
      document.dispatchEvent(new Event('paste'));
    }
  };
  document.queryCommandSupported = s => ['copy', 'paste'].includes(s);
  document[symbols.pointerLockElementSymbol] = null;
  document[symbols.fullscreenElementSymbol] = null;

  const runElQueue = [];
  const _addRun = fn => {
    (async () => {
      if (!document[symbols.runningSymbol]) {
        document[symbols.runningSymbol] = true;

        try {
          await fn();
        } catch(err) {
          console.warn(err.stack);
        }

        document[symbols.runningSymbol] = false;
        if (runElQueue.length > 0) {
          _addRun(runElQueue.shift());
        } else {
          document.emit('flush');
        }
      } else {
        runElQueue.push(fn);
      }
    })();
  };
  document[symbols.runningSymbol] = false;
  document[symbols.addRunSymbol] = _addRun;

  if (window.top === window) {
    document.addEventListener('pointerlockchange', () => {
      const pointerLockElement = document[symbols.pointerLockElementSymbol];
      
      for (let i = 0; i < GlobalContext.contexts.length; i++) {
        const context = GlobalContext.contexts[i];
        nativeBindings.nativeWindow.setCursorMode(context.getWindowHandle(), !pointerLockElement);
      }

      /* const iframes = document.getElementsByTagName('iframe');
      for (let i = 0; i < iframes.length; i++) {
        const iframe = iframes[i];
        if (iframe.contentDocument) {
          // iframe.contentDocument._emit('pointerlockchange'); // XXX send this down
        }
      } */
    });
    document.addEventListener('fullscreenchange', () => {
      const fullscreenElement = document[symbols.fullscreenElementSymbol];
      
      for (let i = 0; i < GlobalContext.contexts.length; i++) {
        const context = GlobalContext.contexts[i];
        nativeBindings.nativeWindow.setFullscreen(context.getWindowHandle(), !!fullscreenElement);
      }
      
      /* const iframes = document.getElementsByTagName('iframe');
      for (let i = 0; i < iframes.length; i++) {
        const iframe = iframes[i];
        if (iframe.contentDocument) {
          // iframe.contentDocument._emit('pointerlockchange'); // XXX send this down
          iframe.contentDocument._emit('fullscreenchange');
        }
      } */
    });
  }

  process.nextTick(async () => {
    const _tryDispatchEvent = (target, event) => {
      try {
        target.dispatchEvent(event);
      } catch(err) {
        console.warn(err);
      }
    };
    
    if (body) {
      const bodyChildNodes = body.childNodes;
      body.childNodes = new window.NodeList();

      try {
        await GlobalContext._runHtml(document.head, window);
      } catch(err) {
        console.warn(err);
      }

      body.childNodes = bodyChildNodes;
      try {
        body._emit('children', Array.from(bodyChildNodes), [], null, null);
      } catch(err) {
        console.warn(err);
      }

      try {
        await GlobalContext._runHtml(document.body, window);
      } catch(err) {
        console.warn(err);
      }

      document.readyState = 'interactive';
      _tryDispatchEvent(document, new Event('readystatechange', {target: document}));

      _tryDispatchEvent(document, new Event('DOMContentLoaded', {
        target: document,
        bubbles: true,
      }));
    } else {
      try {
        await GlobalContext._runHtml(document, window);
      } catch(err) {
        console.warn(err);
      }

      document.readyState = 'interactive';
      _tryDispatchEvent(document, new Event('readystatechange', {target: document}));

      _tryDispatchEvent(document, new Event('DOMContentLoaded', {
        target: document,
        bubbles: true,
      }));
    }

    document.readyState = 'complete';
    _tryDispatchEvent(document, new Event('readystatechange', {target: document}));

    document.dispatchEvent(new Event('load', {target: document}));
    _tryDispatchEvent(window, new Event('load', {target: window}));

    parentPort.postMessage({
      method: 'xrMode',
      xr: GlobalContext.requestedXr,
    });
  });

  return document;
}
module.exports.initDocument = initDocument;

const maxParallelResources = 8;
class Resource {
  constructor(getCb = (onprogress, cb) => cb(), value = 0.5, total = 1) {
    this.getCb = getCb;
    this.value = value;
    this.total = total;
    
    this.onupdate = null;
  }

  setProgress(value) {
    this.value = value;

    this.onupdate && this.onupdate();
  }
  
  get() {
    return new Promise((accept, reject) => {
      this.getCb(progress => {
        this.setValue(progress);
      }, err => {
        if (!err) {
          accept();
        } else {
          reject(err);
        }
      });
    });
  }
  
  destroy() {
    this.setProgress(1);
  }
}
class Resources extends EventTarget {
  constructor() {
    super();
    
    this.resources = [];
    this.queue = [];
    this.numRunning = 0;
  }

  getValue() {
    let value = 0;
    for (let i = 0; i < this.resources.length; i++) {
      value += this.resources[i].value;
    }
    return value;
  }
  getTotal() {
    let total = 0;
    for (let i = 0; i < this.resources.length; i++) {
      total += this.resources[i].total;
    }
    return total;
  }
  getProgress() {
    let value = 0;
    let total = 0;
    for (let i = 0; i < this.resources.length; i++) {
      const resource = this.resources[i];
      value += resource.value;
      total += resource.total;
    }
    return total > 0 ? (value / total) : 1;
  }

  addResource(getCb) {
    return new Promise((accept, reject) => {
      const resource = new Resource(getCb);
      resource.onupdate = () => {
        if (resource.value >= resource.total) {
          this.resources.splice(this.resources.indexOf(resource), 1);
          
          resource.onupdate = null;
          
          accept();
        }

        const e = new Event('update');
        e.value = this.getValue();
        e.total = this.getTotal();
        e.progress = this.getProgress();
        this.dispatchEvent(e);
      };
      this.resources.push(resource);
      this.queue.push(resource);
      
      this.drain();
    });
  }
  
  drain() {
    if (this.queue.length > 0 && this.numRunning < maxParallelResources) {
      const resource = this.queue.shift();
      resource.get()
        .catch(err => {
          console.warn(err.stack);
        })
        .finally(() => {
          resource.destroy();
          
          this.numRunning--;
          
          this.drain();
        });
      
      this.numRunning++;
    } else {
      const _isDone = () => this.numRunning === 0 && this.queue.length === 0;
      if (_isDone()) {
        process.nextTick(() => { // wait one tick for more resources before emitting drain
          if (_isDone()) {
            this.emit('drain');
          }
        });
      }
    }
  }
}
GlobalContext.Resources = Resources;

const _fromAST = (node, window, parentNode, document, uppercase) => {
  if (node.nodeName === '#text') {
    const text = new window.Text(node.value);
    text.parentNode = parentNode;
    return text;
  } else if (node.nodeName === '#comment') {
    const comment = new window.Comment(node.data);
    comment.parentNode = parentNode;
    return comment;
  } else {
    let {tagName} = node;
    if (tagName && uppercase) {
      tagName = tagName.toUpperCase();
    }
    let {attrs, value, content, childNodes, sourceCodeLocation} = node;
    const HTMLElementTemplate = window[symbols.htmlTagsSymbol][tagName];
    const location = sourceCodeLocation  ? {
      line: sourceCodeLocation.startLine,
      col: sourceCodeLocation.startCol,
    } : null;
    const element = HTMLElementTemplate ?
      new HTMLElementTemplate(
        attrs,
        value,
        location,
      )
    :
      new window.HTMLElement(
        tagName,
        attrs,
        value,
        location,
      );
    element.parentNode = parentNode;
    if (!document) { // if there is no document, it's us
      document = element;
      document.defaultView = window;
      window.document = document;
    }
    if (content) {
      element.childNodes = new DOM.NodeList(
        content.childNodes.map(childNode =>
          _fromAST(childNode, window, element, document, uppercase)
        )
      );
    } else if (childNodes) {
      element.childNodes = new DOM.NodeList(
        childNodes.map(childNode =>
          _fromAST(childNode, window, element, document, uppercase)
        )
      );
    }
    return element;
  }
};
module.exports._fromAST = _fromAST;
GlobalContext._fromAST = _fromAST;

function _upgradeElement(window, el, upgradeTagName) {
  const constructor = window.customElements.get(upgradeTagName);
  constructor && window.customElements.upgrade(el, constructor);
}

// To "run" the HTML means to walk it and execute behavior on the elements such as <script src="...">.
// Each candidate element exposes a method on runSymbol which returns whether to await the element load or not.
const _runHtml = (element, window) => {
  if (element instanceof window.HTMLElement) {
    return new Promise((accept, reject) => {
      const {document} = window;

      element.traverse(el => {
        const {id} = el;
        if (id) {
          el._emit('attribute', 'id', id);
        }

        if (el[symbols.runSymbol]) {
          document[symbols.addRunSymbol](el[symbols.runSymbol].bind(el));
        }

        const {tagName} = el;
        if (tagName) {
          if (/\-/.test(tagName)) {
            _upgradeElement(window, el, tagName);
          } else {
            const isAttr = el.getAttribute('is');
            if (isAttr) {
              _upgradeElement(window, el, isAttr);
            }
          }
        }
      });
      if (document[symbols.runningSymbol]) {
        document.once('flush', () => {
          accept();
        });
      } else {
        accept();
      }
    });
  } else {
    return Promise.resolve();
  }
};
module.exports._runHtml = _runHtml;
GlobalContext._runHtml = _runHtml;

class DocumentType {}
module.exports.DocumentType = DocumentType;

class DOMImplementation {
  constructor(window) {
    this._window = window;
  }

  createDocument() {
    throw new Error('not implemented');
  }

  createDocumentType() {
    return new DocumentType();
  }

  createHTMLDocument() {
    return _parseDocument('', this._window);
  }

  hasFeature() {
    return false;
  }
}
module.exports.DOMImplementation = DOMImplementation;

class Document extends DOM.HTMLLoadableElement {
  constructor(window) {
    super(window, 'DOCUMENT');

    this.ownerDocument = null;
    this.hidden = false;
  }

  get nodeType() {
    return DOM.Node.DOCUMENT_NODE;
  }

  get pointerLockElement() {
    return this[symbols.pointerLockElementSymbol];
  }
  set pointerLockElement(pointerLockElement) {}
  get fullscreenElement() {
    return this[symbols.fullscreenElementSymbol];
  }
  set fullscreenElement(fullscreenElement) {}

  exitPointerLock() {
    if (this[symbols.pointerLockElementSymbol] !== null) {
      this[symbols.pointerLockElementSymbol] = null;

      process.nextTick(() => {
        this._emit('pointerlockchange');
      });
    }
  }
  exitFullscreen() {
    if (this[symbols.fullscreenElementSymbol] !== null) {
      this[symbols.fullscreenElementSymbol] = null;

      process.nextTick(() => {
        this._emit('fullscreenchange');
      });
    }
  }
  hasFocus() {
    return this.defaultView.top === this.defaultView;
  }
}
module.exports.Document = Document;
// FIXME: Temporary until refactor out into modules more and not have circular dependencies.
GlobalContext.Document = Document;

class DocumentFragment extends DOM.HTMLElement {
  constructor(window) {
    super(window, 'DOCUMENTFRAGMENT');
  }

  get nodeType() {
    return DOM.Node.DOCUMENT_FRAGMENT_NODE;
  }
}
module.exports.DocumentFragment = DocumentFragment;
GlobalContext.DocumentFragment = DocumentFragment;

class Range extends DocumentFragment {
  constructor(window) {
    super(window, 'RANGE');
  }

  get collapsed() {
    return true;
  }
  get commonAncestorContainer() {
    return this.window.document.body;
  }
  get startContainer() {
    return this.window.document.body;
  }
  get startOffset() {
    return 0;
  }
  get endContainer() {
    return this.window.document.body;
  }
  get endOffset() {
    return 0;
  }

  setStart() {}
  setEnd() {}
  setStartBefore() {}
  setStartAfter() {}
  setEndBefore() {}
  setEndAfter() {}
  selectNode() {}
  selectNodeContents() {}
  collapse() {}
  cloneContents() {}
  deleteContents() {}
  extractContents() {}
  insertNode() {}
  surroundContents() {}
  compareBoundaryPoints() {}
  cloneRange() {
    return new Range(this.window);
  }
  detach() {}
  toString() {
    return '';
  }
  
  createContextualFragment(str) {
    var fragment = this.ownerDocument.createDocumentFragment();
    fragment.innerHTML = str;
    return fragment;
  }
}
module.exports.Range = Range;

const getBoundDocumentElements = window => {
  const bind = (OldClass, makeClass) => {
    const NewClass = makeClass((a, b, c, d) => new OldClass(window, a, b, c, d));
    NewClass.prototype = OldClass.prototype;
    NewClass.constructor = OldClass;
    return NewClass;
  };
  return {
    Document: bind(Document, b => function Document() { return b.apply(this, arguments); }),
    DocumentFragment: bind(DocumentFragment, b => function DocumentFragment() { return b.apply(this, arguments); }),
    Range: bind(Range, b => function Range() { return b.apply(this, arguments); }),
  };
};
module.exports.getBoundDocumentElements = getBoundDocumentElements;
