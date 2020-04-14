const {URL} = require('url');
const stream = require('stream');
const utils = require('./utils');
const windowFetch = require('window-fetch');
const {Response} = windowFetch;
const GlobalContext = require('./GlobalContext');
const { DatArchive } = require('../build/js/DatArchive.bundle.js');
const datFetch = require('dat-fetch')(DatArchive);

const protocols = {
  dat: datFetch
};
['http', 'https', 'file', 'data', 'blob'].forEach(p => {
  protocols[p] = windowFetch;
});
const PROTOCOL_REGEX = /^([^:]*):/;

const _protocolFetch = (u, options) => {
  const match = u.match(PROTOCOL_REGEX);
  const protocol = match ? match[1] : 'http://';
  return protocols[protocol](u, options);
};
async function fetch(u, options) {
  if (typeof u === 'string') {
    const blob = URL.lookupObjectURL(u);
    if (blob) {
      const body = new stream.PassThrough();
      Promise.resolve().then(() => {
        body.end(blob.buffer);
      });
      return new Response(body);
    } else {
      const normalized = utils._normalizeUrl(u, GlobalContext.baseUrl)
      return _protocolFetch(normalized, options);
    }
  } else {
    // Fetch is being called with a Request object
    // We should get the URL for matching the protocol
    return _protocolFetch(u.url, options);
  }
}

// Add expected fetch internals
for (const k in windowFetch) {
  fetch[k] = windowFetch[k];
}

function registerProtocolHandler(protocol, protocolFetchImpl) {
  protocols[protocol] = protocolFetchImpl;
}

module.exports = {
  fetch,
  registerProtocolHandler,
};
