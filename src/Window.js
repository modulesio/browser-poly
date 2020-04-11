const events = require('events');
const {EventEmitter} = events;
const path = require('path');
const fs = require('fs');
const http = require('http');
const https = require('https');
const os = require('os');
const {parentPort} = require('worker_threads');
const util = require('util');
const {TextEncoder, TextDecoder} = util;
const {performance} = require('perf_hooks');
const {
  workerData: {
    args: {
      options,
      id,
      args,
      version,
    },
  },
} = require('worker_threads');

const {SpatialEvent} = require('./Event.js');
const {XRRigidTransform} = require('./XR.js');

const mkdirp = require('mkdirp');
const ws = require('ws');

const {
  /* getUserMedia,
  MediaStream,
  MediaStreamTrack,
  RTCDataChannel, */
  RTCIceCandidate,
  RTCPeerConnection,
  /* RTCPeerConnectionIceEvent,
  RTCRtpReceiver,
  RTCRtpSender, */
  RTCRtpTransceiver,
  RTCSessionDescription,

  RTCPeerConnectionIceEvent,
  RTCDataChannelEvent,
  RTCDataChannelMessageEvent,
  RTCTrackEvent,
} = require('./RTC/index.js');

const {LocalStorage} = require('window-ls');
const indexedDB = require('fake-indexeddb');
const parseXml = require('@rgrove/parse-xml');
const THREE = require('../lib/three-min.js');
const {
  VRDisplay,
  VRFrameData,
  VRPose,
  VRStageParameters,
  FakeXRDisplay,
  Gamepad,
  GamepadButton,
  lookupHMDTypeString,
} = require('./VR.js');

const {maxNumTrackers} = require('./constants');
const GlobalContext = require('./GlobalContext');
const symbols = require('./symbols');

const {
  nativeImage: Image,
  nativeImageData: ImageData,
  nativeImageBitmap: ImageBitmap,
  nativePath2D: Path2D,
  nativeCanvasGradient: CanvasGradient,
  nativeCanvasRenderingContext2D: CanvasRenderingContext2D,
  nativeGl: WebGLRenderingContext,
  nativeGl2: WebGL2RenderingContext,
  nativeAudio: {
    AudioContext,
    AudioNode,
    AudioBufferSourceNode,
    OscillatorNode,
    AudioDestinationNode,
    AudioParam,
    AudioListener,
    GainNode,
    AnalyserNode,
    PannerNode,
    StereoPannerNode,
  },
  nativeVideo: {
    Video,
  },
  nativeOpenVR,
  nativeOculusVR,
  nativeOculusMobileVr,
  nativeMl,
  nativeBrowser,
  nativeWindow,
} = require('./native-bindings');

GlobalContext.id = id;
GlobalContext.args = args;
GlobalContext.version = version;
GlobalContext.baseUrl = options.baseUrl;
GlobalContext.requestedXr = false;

const {_parseDocument, _parseDocumentAst, getBoundDocumentElements, DocumentType, DOMImplementation, initDocument} = require('./Document');
const {
  HTMLElement,
  getBoundDOMElements,
  DOMTokenList,
  NodeList,
  HTMLCollection,
  DOMRect,
  DOMPoint,
  createImageBitmap,
} = require('./DOM');
const {History} = require('./History');
const XR = require('./XR');
const DevTools = require('./DevTools');
const utils = require('./utils');
const {_elementGetter, _elementSetter} = utils;

const isMac = os.platform() === 'darwin';

const localVector = new THREE.Vector3();
const localVector2 = new THREE.Vector3();
const localQuaternion = new THREE.Quaternion();
const localMatrix = new THREE.Matrix4();
const localMatrix2 = new THREE.Matrix4();

const windows = [];
GlobalContext.windows = windows;
const contexts = [];
GlobalContext.contexts = contexts;

const vrPresentState = {
  hmdType: null,
  vrContext: null,
  glContext: null,
  fbo: 0,
  msFbo: 0,
  layers: [],
  responseAccepts: [],
};
GlobalContext.vrPresentState = vrPresentState;

class CustomElementRegistry {
  constructor(window) {
    this._window = window;

    this.elements = {};
    this.extensions = {};
    this.elementPromises = {};
  }

  define(name, constructor, options = {}) {
    name = name.toUpperCase();

    this.elements[name] = constructor;
    if (options.extends) {
      this.extensions[options.extends.toUpperCase()] = name.toLowerCase();
    }

    this._window.document.traverse(el => {
      if (el.tagName === name) {
        this.upgrade(el, constructor);
      }
    });

    const promises = this.elementPromises[name];
    if (promises) {
      for (let i = 0; i < promises.length; i++) {
        promises[i].accept();
      }
      this.elementPromises[name] = null;
    }
  }
  get(name) {
    name = name.toUpperCase();

    return this.elements[name];
  }
  whenDefined(name) {
    name = name.toUpperCase();

    if (this.elements[name]) {
      return Promise.resolve();
    } else {
      let promises = this.elementPromises[name];
      if (!promises) {
        promises = [];
        this.elementPromises[name] = promises;
      }
      const promise = new Promise((accept, reject) => {
        promise.accept = accept;
        promise.reject = reject;
      });
      promises.push(promise);
      return promise;
    }
  }

  upgrade(el, constructor) {
    if (el instanceof HTMLElement) {
      let isConnected = el.isConnected;
      let connectHandled = false;
      el.ownerDocument.on('domchange', () => {
        const newConnected = el.isConnected;
        if (newConnected && !isConnected) {
          el.connectedCallback && el.connectedCallback();
          isConnected = true;
        } else if (isConnected && !newConnected) {
          el.disconnectedCallback && el.disconnectedCallback();
          isConnected = false;
        }
        connectHandled = true;
      });

      const observedAttributes = constructor.observedAttributes || [];
      if (observedAttributes.length > 0) {
        el.on('attribute', (name, value, oldValue) => {
          if (el.attributeChangedCallback && observedAttributes.includes(name)) {
            el.attributeChangedCallback(name, value, oldValue);
          }
        });
      }

      Object.setPrototypeOf(el, constructor.prototype);
      HTMLElement.upgradeElement = el;
      let error = null;
      try {
        Object.setPrototypeOf(el, constructor.prototype);
        Reflect.construct(constructor, []);
      } catch(err) {
        error = err;
      }
      HTMLElement.upgradeElement = null;

      if (!error) {
        if (isConnected && !connectHandled) {
          el.connectedCallback && el.connectedCallback();
        }
      } else {
        throw error;
      }
    } else {
      throw new Error('cannot upgrade non-subclass of HTMLElement');
    }
  }
}

class PaymentRequest {
  constructor(methodData, details, options) {
    this.methodData = methodData;
    this.details = details;
    this.options = options;
  }

  async show() {
    const {methodData, details, options} = this;

    const listeners = window.listeners('paymentrequest');
    if (listeners.length > 0) {
      parentPort.postMessage({
        method: 'paymentRequest',
        event: {
          methodData,
          details,
          options,
        },
      });
    } else {
      throw new Error('no payment request handler');
    }
  }
}

class MonitorManager {
  getList() {
    return nativeWindow.getMonitors();
  }

  select(index) {
    nativeWindow.setMonitor(index);
  }
}

class Screen {
  constructor(window) {
    this._window = window;
  }

  get top() {
    return 0;
  }
  set top(top) {}
  get left() {
    return 0;
  }
  set left(left) {}
  get width() {
    return this._window.innerWidth;
  }
  set width(width) {}
  get height() {
    return this._window.innerHeight;
  }
  set height(height) {}
  get colorDepth() {
    return 24;
  }
  set colorDepth(colorDepth) {}
  get orientation() {
    return {
      angle: 0,
      type: 'landscape-primary',
      onchange: null,
    };
  }
  set orientation(orientation) {}

  get pixelDepth() {
    return this.colorDepth;
  }
  set pixelDepth(pixelDepth) {}
  get availTop() {
    return this.top;
  }
  set availTop(availTop) {}
  get availLeft() {
    return this.left;
  }
  set availLeft(availLeft) {}
  get availWidth() {
    return this.width;
  }
  set availWidth(availWidth) {}
  get availHeight() {
    return this.height;
  }
  set availHeight(availHeight) {}
}

class MediaRecorder extends EventEmitter {
  constructor() {
    super();
  }

  start() {}

  stop() {}

  requestData() {}
}

class DataTransfer {
  constructor({items = [], files = []} = {}) {
    this.items = items;
    this.files = files;
  }
}
class DataTransferItem {
  constructor(kind = 'string', type = 'text/plain', data = null) {
    this.kind = kind;
    this.type = type;
    this.data = data;
  }

  getAsFile() {
    return new Blob([this.data], {
      type: this.type,
    });
  }

  getAsString(callback) {
    const {data} = this;
    setImmediate(() => {
      callback(data);
    });
  }
}

let rafIndex = 0;
const _findFreeSlot = (a, startIndex = 0) => {
  let i;
  for (i = startIndex; i < a.length; i++) {
    if (a[i] === null) {
      break;
    }
  }
  return i;
};
const _makeRequestAnimationFrame = window => (fn, priority = 0) => {
  fn = fn.bind(window);
  fn[symbols.prioritySymbol] = priority;
  const id = ++rafIndex;
  fn[symbols.idSymbol] = id;
  const rafCbs = window[symbols.rafCbsSymbol];
  rafCbs[_findFreeSlot(rafCbs)] = fn;
  rafCbs.sort((a, b) => (b ? b[symbols.prioritySymbol] : 0) - (a ? a[symbols.prioritySymbol] : 0));
  return id;
};

(window => {
  for (const k in EventEmitter.prototype) {
    window[k] = EventEmitter.prototype[k];
  }
  EventEmitter.call(window);

  window.window = window;
  window.self = window;
  window.parent = options.parent || window;
  window.top = options.top || window;

  Object.defineProperty(window, 'innerWidth', {
    get() {
      if (!GlobalContext.xrState.metrics[0]) {
        const screenSize = nativeWindow.getScreenSize();
        GlobalContext.xrState.metrics[0] = screenSize[0]/2;
        // GlobalContext.xrState.metrics[1] = screenSize[1]/2;
      }
      return GlobalContext.xrState.metrics[0];
    },
    set(innerWidth) {
      GlobalContext.xrState.metrics[0] = innerWidth;
    },
  });
  Object.defineProperty(window, 'innerHeight', {
    get() {
      if (!GlobalContext.xrState.metrics[1]) {
        const screenSize = nativeWindow.getScreenSize();
        // GlobalContext.xrState.metrics[0] = screenSize[0]/2;
        GlobalContext.xrState.metrics[1] = screenSize[1]/2;
      }
      return GlobalContext.xrState.metrics[1];
    },
    set(innerHeight) {
      GlobalContext.xrState.metrics[1] = innerHeight;
    },
  });
  Object.defineProperty(window, 'devicePixelRatio', {
    get() {
      if (!GlobalContext.xrState.devicePixelRatio[0]) {
        GlobalContext.xrState.devicePixelRatio[0] = nativeWindow.getDevicePixelRatio();
      }
      return GlobalContext.xrState.devicePixelRatio[0];
    },
    set(devicePixelRatio) {},
  });
  window.document = null;
  window.history = new History(location.href);
  window.matchMedia = media => ({
    media,
    matches: false,
  });

  // WebVR enabled.
  if (['all', 'webvr'].includes(options.args.xr)) {
    window.navigator.getVRDisplays = function() {
      return Promise.resolve(this.getVRDisplaysSync());
    }
  }

  // WebXR enabled.
  if (['all', 'webxr'].includes(options.args.xr)) {
    window.navigator.xr = new XR.XR(window);
  }

  window.alert = console.log;
  window.setTimeout = (setTimeout => (fn, timeout, args) => {
    fn = fn.bind.apply(fn, [window].concat(args));
    const id = _findFreeSlot(timeouts, 1);
    timeouts[id] = fn;
    fn[symbols.timeoutSymbol] = setTimeout(fn, timeout, args);
    return id;
  })(setTimeout);
  window.clearTimeout = (clearTimeout => id => {
    const fn = timeouts[id];
    if (fn) {
      clearTimeout(fn[symbols.timeoutSymbol]);
      timeouts[id] = null;
    }
  })(clearTimeout);
  window.setInterval = (setInterval => (fn, interval, args) => {
    if (interval < 10) {
      interval = 10;
    }
    fn = fn.bind.apply(fn, [window].concat(args));
    const id = _findFreeSlot(intervals, 1);
    intervals[id] = fn;
    fn[symbols.timeoutSymbol] = setInterval(fn, interval, args);
    return id;
  })(setInterval);
  window.clearInterval = (clearInterval => id => {
    const fn = intervals[id];
    if (fn) {
      clearInterval(fn[symbols.timeoutSymbol]);
      intervals[id] = null;
    }
  })(clearInterval);
  window.event = null;
  window.localStorage = new LocalStorage(path.join(options.dataPath, '.localStorage'));
  window.sessionStorage = new LocalStorage(path.join(options.dataPath, '.sessionStorage'));
  window.indexedDB = indexedDB;
  window.screen = new Screen(window);
  window.scrollTo = function(x = 0, y = 0) {
    this.scrollX = x;
    this.scrollY = y;
  };
  window.scrollX = 0;
  window.scrollY = 0;
  window[symbols.optionsSymbol] = options;
  window[symbols.styleEpochSymbol] = 0;

  // DOM.
  const {
    Document,
    DocumentFragment,
    Range,
  } = getBoundDocumentElements(window);
  window.Document = Document;
  window.DocumentFragment = DocumentFragment;
  window.DocumentType = DocumentType;
  window.DOMImplementation = DOMImplementation;
  window.Range = Range;

  const {
    Element,
    HTMLElement,
    HTMLHeadElement,
    HTMLBodyElement,
    HTMLAnchorElement,
    HTMLStyleElement,
    HTMLLinkElement,
    HTMLScriptElement,
    HTMLImageElement,
    HTMLAudioElement,
    HTMLVideoElement,
    HTMLSourceElement,
    SVGElement,
    HTMLIFrameElement,
    HTMLCanvasElement,
    HTMLTextareaElement,
    HTMLTemplateElement,
    HTMLDivElement,
    HTMLUListElement,
    HTMLLIElement,
    HTMLTableElement,
    Node,
    Text,
    Comment,
  } = getBoundDOMElements(window);
  window.Element = Element;
  window.HTMLElement = HTMLElement;
  window.HTMLHeadElement = HTMLHeadElement;
  window.HTMLBodyElement = HTMLBodyElement;
  window.HTMLAnchorElement = HTMLAnchorElement;
  window.HTMLStyleElement = HTMLStyleElement;
  window.HTMLLinkElement = HTMLLinkElement;
  window.HTMLScriptElement = HTMLScriptElement;
  window.HTMLImageElement = HTMLImageElement,
  window.HTMLAudioElement = HTMLAudioElement;
  window.HTMLVideoElement = HTMLVideoElement;
  window.SVGElement = SVGElement;
  window.HTMLIFrameElement = HTMLIFrameElement;
  window.HTMLCanvasElement = HTMLCanvasElement;
  window.HTMLTextareaElement = HTMLTextareaElement;
  window.HTMLTemplateElement = HTMLTemplateElement;
  window.HTMLDivElement = HTMLDivElement;
  window.HTMLUListElement = HTMLUListElement;
  window.HTMLLIElement = HTMLLIElement;
  window.HTMLTableElement = HTMLTableElement;
  window.Node = Node;
  window.Text = Text;
  window.Comment = Comment;
  window[symbols.htmlTagsSymbol] = {
    DOCUMENT: Document,
    HEAD: HTMLHeadElement,
    BODY: HTMLBodyElement,
    A: HTMLAnchorElement,
    STYLE: HTMLStyleElement,
    SCRIPT: HTMLScriptElement,
    LINK: HTMLLinkElement,
    IMG: HTMLImageElement,
    AUDIO: HTMLAudioElement,
    VIDEO: HTMLVideoElement,
    SOURCE: HTMLSourceElement,
    IFRAME: HTMLIFrameElement,
    CANVAS: HTMLCanvasElement,
    TEXTAREA: HTMLTextareaElement,
    TEMPLATE: HTMLTemplateElement,
    DIV: HTMLDivElement,
    ULIST: HTMLUListElement,
    LI: HTMLLIElement,
    TABLE: HTMLTableElement,
  };
  window.DOMTokenList = DOMTokenList;
  window.NodeList = NodeList;
  window.HTMLCollection = HTMLCollection;

  /* window.MediaStreamTrack = MediaStreamTrack;
  window.RTCRtpReceiver = RTCRtpReceiver;
  window.RTCRtpSender = RTCRtpSender; */
  window.MediaStream = class MediaStream {
    getAudioTracks() {
      return [];
    }
    getVideoTracks() {
      return [];
    }
  };

  window.RTCPeerConnection = RTCPeerConnection;
  window.webkitRTCPeerConnection = RTCPeerConnection; // for feature detection
  window.RTCSessionDescription = RTCSessionDescription;
  window.RTCIceCandidate = RTCIceCandidate;

  window.RTCPeerConnectionIceEvent = RTCPeerConnectionIceEvent;
  window.RTCDataChannelEvent = RTCDataChannelEvent;
  window.RTCDataChannelMessageEvent = RTCDataChannelMessageEvent;
  window.RTCTrackEvent = RTCTrackEvent;

  window.RTCRtpTransceiver = RTCRtpTransceiver;

  window.customElements = new CustomElementRegistry(window);
  window.CustomElementRegistry = CustomElementRegistry;
  window.MutationObserver = require('./MutationObserver').MutationObserver;
  window.DOMRect = DOMRect;
  window.DOMPoint = DOMPoint;
  window.getComputedStyle = el => {
    let styleSpec = el[symbols.computedStyleSymbol];
    if (!styleSpec || styleSpec.epoch !== window[symbols.styleEpochSymbol]) {
      const style = el.style.clone();
      const stylesheetEls = el.ownerDocument.documentElement.getElementsByTagName('style')
        .concat(el.ownerDocument.documentElement.getElementsByTagName('link'));
      for (let i = 0; i < stylesheetEls.length; i++) {
        const {stylesheet} = stylesheetEls[i];
        if (stylesheet) {
          const {rules} = stylesheet;
          for (let j = 0; j < rules.length; j++) {
            const rule = rules[j];
            const {selectors} = rule;
            if (selectors && selectors.some(selector => el.matches(selector))) {
              const {declarations} = rule;
              for (let k = 0; k < declarations.length; k++) {
                const {property, value} = declarations[k];
                style[property] = value;
              }
            }
          }
        }
      }
      styleSpec = {
        style,
        styleEpoch: window[symbols.styleEpochSymbol],
      };
      el[symbols.computedStyleSymbol] = styleSpec;
    }
    return styleSpec.style;
  };
  window.browser = {
    devTools: DevTools,
    http,
    // https,
    ws,
    magicleap: nativeMl ? {
      RequestMeshing: () => nativeMl.RequestMeshing(window),
      RequestPlaneTracking: () => nativeMl.RequestPlaneTracking(window),
      RequestHandTracking: () => nativeMl.RequestHandTracking(window),
      RequestEyeTracking: () => nativeMl.RequestEyeTracking(window),
      RequestImageTracking: (img, size) => nativeMl.RequestImageTracking(window, img, size),
      RequestDepthPopulation: nativeMl.RequestDepthPopulation,
      RequestCamera: nativeMl.RequestCamera,
    } : null,
    monitors: new MonitorManager(),
    setSetting(key, value) {
      args[key] = value;
    },
    inspect: util.inspect,
    requestDomExport(el) {
      const promises = [];
      const _getExport = el => {
        if (el.nodeType === Node.ELEMENT_NODE && el.tagName === 'IFRAME' && el.contentWindow) {
          if (el.contentWindow.evalAsync) {
            const promise = el.contentWindow.evalAsync(`browser.requestDomExport(document.body.parentNode)`)
              .then(iframeResult => {
                result.childNodes = [iframeResult];
              });
            promises.push(promise);
          }
        }

        const result = {
          nodeType: el.nodeType || 0,
          tagName: el.tagName || '',
          value: el.value || '',
          attrs: el.attrs || [],
          childNodes: el.childNodes.map(_getExport),
        };
        return result;
      };
      const result = _getExport(el);
      return Promise.all(promises).then(() => result);
    },
    async applyDomEdit(rootEl, keypath, edit) {
      const _getDomNode = (el, i = 0) => {
        if (i < keypath.length) {
          const key = keypath[i];
          const childNode = el.childNodes[key];
          if (childNode) {
            return _getDomNode(childNode, i+1);
          } else {
            return [el, keypath.slice(i)];
          }
        } else {
          return [el, []];
        }
      };
      const [el, remainingKeypath] = _getDomNode(rootEl);
      if (remainingKeypath.length === 0) {
        const {type} = edit;
        if (type === 'name') {
          const {oldName, oldValue, newName} = edit;
          el.removeAttribute(oldName);
          el.setAttribute(newName, oldValue);
        } else if (type === 'value') {
          const {name, newValue} = edit;
          el.setAttribute(name, newValue);
        } else if (type === 'remove') {
          el.parentNode.removeChild(el);
        } else {
          throw new Error(`unknown dom edit type: ${type}`);
        }
      } else {
        if (el.tagName === 'IFRAME' && el.contentWindow) {
          await el.contentWindow.evalAsync(`browser.applyDomEdit(document, ${JSON.stringify(remainingKeypath)}, ${JSON.stringify(edit)})`);
        } else {
          console.warn('unresolved dom edit', el, remainingKeypath);
        }
      }
    },
  };
  window.DOMParser = class DOMParser {
    parseFromString(htmlString, type) {
      const _recurse = node => {
        let nodeName = null;
        let value = null;
        if (node.type === 'text') {
          nodeName = '#text';
          value = node.text;
        } else if (node.type === 'comment') {
          nodeName = '#comment';
          value = node.content;
        }

        const tagName = node.name || null;

        const attrs = [];
        if (node.attributes) {
          for (const name in node.attributes) {
            attrs.push({
              name,
              value: node.attributes[name],
            });
          }
        }

        const childNodes = node.children ? node.children.map(childNode => _recurse(childNode)) : [];

        return {
          nodeName,
          tagName,
          attrs,
          value,
          childNodes,
        };
      };
      const xmlAst = parseXml(htmlString, {
        // preserveComments: true,
      });
      const htmlAst = _recurse(xmlAst);
      return _parseDocumentAst(htmlAst, window, false);
    }
  };
  window.addEventListener = function(e, fn, opts) {
    if (e === 'vrdisplayactivate') {
      GlobalContext.requestedXr = true;
    }
    return EventTarget.prototype.addEventListener.apply(window, arguments);
  };
  window.removeEventListener = EventTarget.prototype.removeEventListener.bind(window);
  window.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(window);
  window.Image = HTMLImageElement;
  window.ImageData = ImageData;
  window.ImageBitmap = ImageBitmap;
  window.Path2D = Path2D;
  window.CanvasGradient = CanvasGradient;
  window.CanvasRenderingContext2D = CanvasRenderingContext2D;
  window.WebGLRenderingContext = WebGLRenderingContext;
  if (options.args.webgl !== '1') {
    window.WebGL2RenderingContext = WebGL2RenderingContext;
  }
  window.Audio = HTMLAudioElement;
  window.MediaRecorder = MediaRecorder;
  window.DataTransfer = DataTransfer;
  window.DataTransferItem = DataTransferItem;
  window.Screen = Screen;
  window.Gamepad = Gamepad;
  window.VRStageParameters = VRStageParameters;
  window.VRDisplay = VRDisplay;
  // window.ARDisplay = ARDisplay;
  window.VRFrameData = VRFrameData;
  if (window.navigator.xr) {
    window.XR = XR.XR;
    // window.XRDevice = XR.XRDevice;
    window.XRSession = XR.XRSession;
    window.XRRenderState = XR.XRRenderState;
    window.XRWebGLLayer = XR.XRWebGLLayer;
    window.XRFrame = XR.XRFrame;
    window.XRView = XR.XRView;
    window.XRViewport = XR.XRViewport;
    window.XRPose = XR.XRPose;
    window.XRViewerPose = XR.XRViewerPose;
    window.XRInputSource = XR.XRInputSource;
    window.XRRay = XR.XRRay;
    window.XRInputPose = XR.XRInputPose;
    window.XRInputSourceEvent = XR.XRInputSourceEvent;
    window.XRSpace = XR.XRSpace;
    window.XRReferenceSpace = XR.XRReferenceSpace;
    window.XRBoundedReferenceSpace = XR.XRBoundedReferenceSpace;
  }
  window.FakeXRDisplay = FakeXRDisplay;
  window.TextEncoder = TextEncoder;
  window.TextDecoder = TextDecoder;
  window.AudioContext = AudioContext;
  window.AudioNode = AudioNode;
  window.AudioBufferSourceNode = AudioBufferSourceNode;
  window.OscillatorNode = OscillatorNode;
  window.AudioDestinationNode = AudioDestinationNode;
  window.AudioParam = AudioParam;
  window.AudioListener = AudioListener;
  window.GainNode = GainNode;
  window.AnalyserNode = AnalyserNode;
  window.PannerNode = PannerNode;
  window.StereoPannerNode = StereoPannerNode;
  window.createImageBitmap = createImageBitmap;
  window.Worker = Worker;
  window.PaymentRequest = PaymentRequest;
  window.requestAnimationFrame = _makeRequestAnimationFrame(window);
  window.cancelAnimationFrame = id => {
    const index = rafCbs.findIndex(r => r && r[symbols.idSymbol] === id);
    if (index !== -1) {
      rafCbs[index] = null;
    }
  };
  window.postMessage = (postMessage => function(data) {
    if (window.top === window) {
      setImmediate(() => {
        window._emit('message', new MessageEvent('message', {data}));
      });
    } else {
      postMessage.apply(this, arguments);
    }
  })(window.postMessage);
  /*
    Treat function onload() as a special case that disables automatic event attach for onload, because this is how browsers work. E.g.
      <!doctype html><html><head><script>
        function onload() {
          console.log ('onload'); // NOT called; presence of top-level function onload() makes all the difference
        }
        window.onload = onload;
      </script></head></html>
  */
  window[symbols.disabledEventsSymbol] = {
    load: undefined,
    error: undefined,
  };
  window._emit = function(type) {
    if (!this[symbols.disabledEventsSymbol][type]) {
      Node.prototype._emit.apply(this, arguments);
    }
  };
  Object.defineProperty(window, 'onload', {
    get() {
      return _elementGetter(window, 'load');
    },
    set(onload) {
      _elementSetter(window, 'load', onload);
    },
  });
  Object.defineProperty(window, 'onerror', {
    get() {
      return _elementGetter(window, 'error');
    },
    set(onerror) {
      _elementSetter(window, 'error', onerror);
    },
  });
  Object.defineProperty(window, 'onpopstate', {
    get() {
      return _elementGetter(window, 'popstate');
    },
    set(onpopstate) {
      _elementSetter(window, 'popstate', onpopstate);
    },
  });

  window.history.on('popstate', (u, state) => {
    window.location.set(u);

    const event = new Event('popstate');
    event.state = state;
    window.dispatchEvent(event);
  });
  let loading = false;
  window.location.on('update', href => {
    if (!loading) {
      loading = true;

      window._emit('beforeunload');
      window._emit('unload');

      parentPort.postMessage({
        method: 'emit',
        type: 'navigate',
        event: {
          href,
        },
      });
    }
  });

  const rafCbs = [];
  window[symbols.rafCbsSymbol] = rafCbs;
  const timeouts = [null];
  const intervals = [null];
  const localCbs = [];
  const prevSyncs = [];
  const _cacheLocalCbs = cbs => {
    for (let i = 0; i < cbs.length; i++) {
      localCbs[i] = cbs[i];
    }
    for (let i = cbs.length; i < localCbs.length; i++) {
      localCbs[i] = null;
    }
  };
  const _clearLocalCbs = () => {
    for (let i = 0; i < localCbs.length; i++) {
      localCbs[i] = null;
    }
  };
  const _clearPrevSyncs = () => {
    for (let i = 0; i < prevSyncs.length; i++) {
      nativeWindow.deleteSync(prevSyncs[i]);
    }
    prevSyncs.length = 0;
  };
  const _bindXrFramebuffer = layered => {
    if (vrPresentState.glContext) {
      nativeWindow.setCurrentWindowContext(vrPresentState.glContext.getWindowHandle());

      if (layered) {
        if (GlobalContext.xrState.aaEnabled[0]) {
          vrPresentState.glContext.setDefaultFramebuffer(vrPresentState.msFbo);
          nativeWindow.bindVrChildMsFbo(vrPresentState.glContext, vrPresentState.msFbo, GlobalContext.xrState.msTex[0], GlobalContext.xrState.msDepthTex[0]);
        } else {
          vrPresentState.glContext.setDefaultFramebuffer(vrPresentState.fbo);
          nativeWindow.bindVrChildFbo(vrPresentState.glContext, vrPresentState.fbo, GlobalContext.xrState.tex[0], GlobalContext.xrState.depthTex[0]);
        }
        
        vrPresentState.glContext.setClearEnabled(false);
      } else {
        if (GlobalContext.xrState.aaEnabled[0]) {
          vrPresentState.glContext.setDefaultFramebuffer(vrPresentState.glContext.framebuffer.msFbo);
          nativeWindow.bindVrChildFbo(vrPresentState.glContext, vrPresentState.glContext.framebuffer.msFbo, vrPresentState.glContext.framebuffer.msTex, vrPresentState.glContext.framebuffer.msDepthTex);
        } else {
          vrPresentState.glContext.setDefaultFramebuffer(vrPresentState.glContext.framebuffer.fbo);
          nativeWindow.bindVrChildFbo(vrPresentState.glContext, vrPresentState.glContext.framebuffer.fbo, vrPresentState.glContext.framebuffer.tex, vrPresentState.glContext.framebuffer.depthTex);
        }

        vrPresentState.glContext.setClearEnabled(true);
      }
    }
  };
  const _emitXrEvents = () => {
    if (window[symbols.mrDisplaysSymbol].xrSession.isPresenting) {
      window[symbols.mrDisplaysSymbol].xrSession.update();
    }
  };
  const _waitLocalSyncs = syncs => {
    if (vrPresentState.glContext) {
      nativeWindow.setCurrentWindowContext(vrPresentState.glContext.getWindowHandle());
      for (let i = 0; i < syncs.length; i++) {
        const sync = syncs[i];
        nativeWindow.waitSync(sync);
        prevSyncs.push(sync);
      }
      return [];
    } else {
      if (nativeWindow.hasCurrentWindowContext()) {
        for (let i = 0; i < syncs.length; i++) {
          nativeWindow.deleteSync(syncs[i]);
        }
        return [];
      } else {
        return syncs;
      }
    }
  };
  const _tickLocalRafs = () => {
    if (rafCbs.length > 0) {
      _cacheLocalCbs(rafCbs);
      
      const performanceNow = performance.now();

      for (let i = 0; i < localCbs.length; i++) {
        const rafCb = localCbs[i];
        if (rafCb) {
          try {
            rafCb(performanceNow);
          } catch (e) {
            console.warn(e);
          }

          rafCbs[i] = null;
        }
      }

      _clearLocalCbs(); // release garbage
    }
  };
  const _composeXrContext = (context, windowHandle, layered) => {
    // compose non-window layers
    if (vrPresentState.glContext) {
      nativeWindow.composeLayers(vrPresentState.glContext, vrPresentState.layers, GlobalContext.xrState);
    }

    // downsample unlayered framebuffers
    if (!layered && GlobalContext.xrState.aaEnabled[0]) {
      const width = GlobalContext.xrState.renderWidth[0]*2;
      const height = GlobalContext.xrState.renderHeight[0];
      const {msTex, msDepthTex, tex, depthTex} = vrPresentState.glContext.framebuffer;

      nativeWindow.blitChildFrameBuffer(context, msTex, msDepthTex, true, tex, depthTex, false, width, height, width, height);
    }

    // blit to window
    if (!context.canvas.ownerDocument.hidden) {
      if (vrPresentState.hmdType === 'fake' || vrPresentState.hmdType === 'oculus' || vrPresentState.hmdType === 'openvr') {
        const width = GlobalContext.xrState.renderWidth[0]*2;
        const height = GlobalContext.xrState.renderHeight[0];
        const {width: dWidth, height: dHeight} = nativeWindow.getFramebufferSize(windowHandle);
        nativeWindow.blitChildFrameBuffer(context, GlobalContext.xrState.tex[0], 0, false, 0, 0, false, width, height, dWidth, dHeight);
      }
      
      _swapBuffers(context, windowHandle);
    }
  };
  const _composeNormalContext = (context, windowHandle) => {
    if (!context.canvas.ownerDocument.hidden) {
      const {canvas: {width, height}, framebuffer: {msFbo, msTex}} = context;
      if (msFbo !== 0) {
        nativeWindow.blitChildFrameBuffer(context, msTex, 0, true, 0, 0, false, width, height, width, height);
      }
      _swapBuffers(context, windowHandle);
    }
  };
  const _swapBuffers = (context, windowHandle) => {
    if (isMac) {
      context.bindFramebufferRaw(context.FRAMEBUFFER, null);
    }
    nativeWindow.swapBuffers(windowHandle);
  };
  const _composeLocalLayers = layered => {
    const syncs = [];

    for (let i = 0; i < contexts.length; i++) {
      const context = contexts[i];
      const isDirty = (!!context.isDirty && context.isDirty()) || context === vrPresentState.glContext;
      if (isDirty) {
        // if (layered) {
          const windowHandle = context.getWindowHandle();

          nativeWindow.setCurrentWindowContext(windowHandle);
          if (isMac) {
            context.flush();
          }

          if (context === vrPresentState.glContext) {
            _composeXrContext(context, windowHandle, layered);
          } else {
            _composeNormalContext(context, windowHandle);
          }

          if (isMac) {
            const drawFramebuffer = context.getBoundFramebuffer(context.DRAW_FRAMEBUFFER);
            if (drawFramebuffer) {
              context.bindFramebuffer(context.DRAW_FRAMEBUFFER, drawFramebuffer);
            }

            const readFramebuffer = context.getBoundFramebuffer(context.READ_FRAMEBUFFER);
            if (readFramebuffer) {
              context.bindFramebuffer(context.READ_FRAMEBUFFER, readFramebuffer);
            }
          }
        // }
        
        context.clearDirty();

        if (context.finish) {
          syncs.push(nativeWindow.getSync());
        }
      }
    }

    return syncs;
  };
  const _renderLocal = (syncs, layered) => {
    syncs = _waitLocalSyncs(syncs);
    _tickLocalRafs();
    syncs.push.apply(syncs, _composeLocalLayers(layered));
    return Promise.resolve(syncs);
  };
  const _makeRenderChild = window => (syncs, layered) => window.runAsync({
    method: 'tickAnimationFrame',
    syncs,
    layered: layered && vrPresentState.layers.some(layer => layer.contentWindow === window),
  });
  const _collectRenders = () => windows.map(_makeRenderChild).concat([_renderLocal]);
  const _render = (syncs, layered) => new Promise((accept, reject) => {
    const renders = _collectRenders();
    const _recurse = i => {
      if (i < renders.length) {
        renders[i](syncs, layered)
          .then(newSyncs => {
            syncs = newSyncs;
            _recurse(i+1);
          })
          .catch(err => {
            console.warn('failed to render child', err);
            syncs = [];
            _recurse(i+1);
          });
      } else {
        accept(syncs);
      }
    };
    _recurse(0);
  });
  window.tickAnimationFrame = async ({syncs = [], layered = false}) => {
    _clearPrevSyncs();
    _bindXrFramebuffer(layered);
    _emitXrEvents(); 
    return _render(syncs, layered);
  };

  const _makeMrDisplays = () => {
    const _onrequestpresent = async () => {
      GlobalContext.requestedXr = true;
      
      // if (!GlobalContext.xrState.isPresenting[0]) {
        await new Promise((accept, reject) => {
          vrPresentState.responseAccepts.push(accept);

          parentPort.postMessage({
            method: 'request',
            type: 'requestPresent',
            keypath: [],
          });
        });
      // }

      vrPresentState.hmdType = lookupHMDTypeString(GlobalContext.xrState.hmdType[0]);
      GlobalContext.clearGamepads();
    };
    const _onmakeswapchain = context => {
      if (context !== vrPresentState.glContext) {
        /* if (vrPresentState.glContext) {
          vrPresentState.glContext.setClearEnabled(true);
        } */

        vrPresentState.glContext = context;
        vrPresentState.fbo = context.createFramebuffer().id;
        vrPresentState.msFbo = context.createFramebuffer().id;
        // vrPresentState.glContext.setClearEnabled(false);

        window.document.emit('domchange'); // open mirror window
      }

      return {
        fbo: vrPresentState.fbo,
      };
    };
    const _onexitpresent = async () => {
      // if (GlobalContext.xrState.isPresenting[0]) {
        await new Promise((accept, reject) => {
          vrPresentState.responseAccepts.push(accept);

          parentPort.postMessage({
            method: 'request',
            type: 'exitPresent',
            keypath: [],
          });
        });
      // }

      vrPresentState.hmdType = null;
      // vrPresentState.glContext.setClearEnabled(true);
      vrPresentState.glContext = null;
      GlobalContext.clearGamepads();
    };
    const _onrequesthittest = (origin, direction, coordinateSystem) => new Promise((accept, reject) => {
      localQuaternion.setFromUnitVectors(
        localVector.set(0, 0, -1),
        localVector2.fromArray(direction)
      );
      localMatrix.compose(
        localVector.fromArray(origin),
        localQuaternion,
        localVector2.set(1, 1, 1)
      ).premultiply(
        localMatrix2.fromArray(window.document.xrOffset.matrix)
      ).decompose(
        localVector,
        localQuaternion,
        localVector2
      );
      localVector.toArray(origin);
      localVector2.set(0, 0, -1).applyQuaternion(localQuaternion).toArray(direction);
      vrPresentState.responseAccepts.push(res => {
        const {error, result} = res;
        if (!error) {
          localMatrix.fromArray(window.document.xrOffset.matrixInverse);

          for (let i = 0; i < result.length; i++) {
            const {hitMatrix} = result[i];
            localMatrix2
              .fromArray(hitMatrix)
              .premultiply(localMatrix)
              .toArray(hitMatrix);
          }

          accept(result);
        } else {
          reject(error);
        }
      });

      parentPort.postMessage({
        method: 'request',
        type: 'requestHitTest',
        keypath: [],
        origin,
        direction,
        coordinateSystem,
      });
    });

    const vrDisplay = new VRDisplay('OpenVR', window);
    vrDisplay.onrequestanimationframe = _makeRequestAnimationFrame(window);
    vrDisplay.oncancelanimationframe = window.cancelAnimationFrame;
    vrDisplay.onvrdisplaypresentchange = () => {
      const e = new Event('vrdisplaypresentchange');
      e.display = vrDisplay;
      window.dispatchEvent(e);
    };
    vrDisplay.onrequestpresent = _onrequestpresent;
    vrDisplay.onmakeswapchain = _onmakeswapchain;
    vrDisplay.onexitpresent = _onexitpresent;
    vrDisplay.onlayers = layers => {
      vrPresentState.layers = layers;
    };
    
    const xrSession = new XR.XRSession({}, window);
    xrSession.onrequestpresent = (onrequestpresent => function() {
      vrDisplay.isPresenting = true;
      xrSession.once('end', () => {
        vrDisplay.isPresenting = false;
      });
      return onrequestpresent.apply(this, arguments);
    })(_onrequestpresent);
    xrSession.onmakeswapchain = _onmakeswapchain;
    xrSession.onexitpresent = _onexitpresent;
    xrSession.onrequestanimationframe = _makeRequestAnimationFrame(window);
    xrSession.oncancelanimationframe = window.cancelAnimationFrame;
    xrSession.onrequesthittest = _onrequesthittest;
    xrSession.onlayers = layers => {
      vrPresentState.layers = layers;
    };

    return {
      vrDisplay,
      xrSession,
    };
  };
  window[symbols.mrDisplaysSymbol] = _makeMrDisplays();
  window.vrdisplayactivate = () => {
    const displays = window.navigator.getVRDisplaysSync();
    if (displays.length > 0 && (!window[symbols.optionsSymbol].args || ['all', 'webvr'].includes(window[symbols.optionsSymbol].args.xr)) && !displays[0].isPresenting) {
      const e = new window.Event('vrdisplayactivate');
      e.display = displays[0];
      window.dispatchEvent(e);
    }
  };

  window.document = _parseDocument(options.htmlString, window);
  window.document.hidden = options.hidden || false;
  window.document.xrOffset = options.xrOffsetBuffer ? new XRRigidTransform(options.xrOffsetBuffer) : new XRRigidTransform();
})(global);

global.onrunasync = req => {
  const {method} = req;

  switch (method) {
    case 'tickAnimationFrame':
      return global.tickAnimationFrame(req);
    case 'response': {
      const {keypath} = req;

      if (keypath.length === 0) {
        if (vrPresentState.responseAccepts.length > 0) {
          vrPresentState.responseAccepts.shift()(req);

          return Promise.resolve();
        } else {
          return Promise.reject(new Error(`unexpected response at window ${method}`));
        }
      } else {
        const windowId = keypath.pop();
        const window = windows.find(window => window.id === windowId);

        if (window) {
          window.runAsync({
            method: 'response',
            keypath,
            error: req.error,
            result: req.result,
          });
        } else {
          console.warn('ignoring unknown response', req, {windowId});
        }
        return Promise.resolve();
      }
    }
    case 'keyEvent': {
      const {event} = request;
      switch (event.type) {
        case 'keydown':
        case 'keypress':
        case 'keyup': {
          if (vrPresentState.glContext) {
            const {canvas} = vrPresentState.glContext;
            canvas.dispatchEvent(new global.KeyboardEvent(event.type, event));
          }
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case 'meshes': {
      for (let i = 0; i < windows.length; i++) {
        windows[i].runAsync(req);
      }

      const {xrOffset} = global.document;

      if (window[symbols.mrDisplaysSymbol].xrSession.isPresenting) {
        for (let i = 0; i < req.updates.length; i++) {
          const update = req.updates[i];

          if (update.transformMatrix && xrOffset) {
            localMatrix
              .fromArray(update.transformMatrix)
              .premultiply(
                localMatrix2.fromArray(xrOffset.matrixInverse)
              )
              .toArray(update.transformMatrix);          
          }
          const e = new SpatialEvent(update.type, {
            detail: {
              update,
            },
          });
          window[symbols.mrDisplaysSymbol].xrSession.dispatchEvent(e);
        }
      }
      break;
    }
    case 'planes': {
      for (let i = 0; i < windows.length; i++) {
        windows[i].runAsync(req);
      }
      
      const {xrOffset} = global.document;
      if (xrOffset) {
        localMatrix.fromArray(xrOffset.matrixInverse);
      }
      
      if (window[symbols.mrDisplaysSymbol].xrSession.isPresenting) {
        for (let i = 0; i < req.updates.length; i++) {
          const update = req.updates[i];
          if (update.position && xrOffset) {
            localVector.fromArray(update.position).applyMatrix4(localMatrix).toArray(update.position);
            localVector.fromArray(update.normal).applyQuaternion(localQuaternion).toArray(update.normal);
          }
          const e = new SpatialEvent(update.type, {
            detail: {
              update,
            },
          });
          window[symbols.mrDisplaysSymbol].xrSession.dispatchEvent(e);
        }
      }
      break;
    }
    case 'eval': // used in tests
      return Promise.resolve(eval(req.scriptString));
    default:
      return Promise.reject(new Error(`invalid window async request: ${JSON.stringify(req)}`));
  }
};
global.onexit = () => {
  const localContexts = contexts.slice();
  for (let i = 0; i < localContexts.length; i++) {
    localContexts[i].destroy();
  }
  
  AudioContext.Destroy();
  nativeWindow.destroyThreadPool();
};
// global.setImmediate = undefined; // need this for the TLS implementation
