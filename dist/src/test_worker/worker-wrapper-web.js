// Web helper to create a worker that initializes tools.js via Blob.
function createWorkerFromTools(options = {}) {
  const base = options.baseUrl || './utils/';
  const toolsUrl = base.replace(/\/$/, '') + '/tools.js'.replace(/\/\//g, '/');

  const script = `
    try {
      importScripts('${base}tools.js');
    } catch (e) {
      self.postMessage({ type: 'error', error: 'importScripts_failed', detail: String(e) });
    }
    (async function(){
      const tools = (typeof self.__TOOLS_UTILS_EXPORTS__ !== 'undefined') ? self.__TOOLS_UTILS_EXPORTS__ : null;
      if (!tools) { self.postMessage({ type: 'error', error: 'no_tools_exports' }); return; }
      try {
        self.postMessage({ type: 'after_importscripts' });
        self.postMessage({ type: 'tools_global_exists', exists: true });
        self.postMessage({ type: 'init_started' });
        try {
          const p = tools.init({ baseUrl: '${base}' });
          if (p && typeof p.then === 'function') {
            p.then(function(){ self.postMessage({ type: 'init_done' }); }).catch(function(e){ self.postMessage({ type: 'init_failed', detail: String(e) }); });
            await p.catch(function(){ /* handled above */ });
          } else {
            self.postMessage({ type: 'init_done_sync' });
          }
        } catch (e) { self.postMessage({ type:'init_failed', detail:String(e) }); }
      } catch(e){ self.postMessage({ type:'init_failed', detail:String(e) }); }
      self.postMessage({ type: 'init', hasScrFix: typeof tools.scr_fix === 'function', hasInvertAlg: typeof tools.invertAlg === 'function' });
      try { if (typeof tools.scr_fix === 'function') self.postMessage({ type: 'scr_fix', output: tools.scr_fix("R U R' U'") }); } catch(e){ self.postMessage({ type:'error', detail:String(e) }); }
      try { if (typeof tools.invertAlg === 'function') { const inv = await tools.invertAlg('R U R2'); self.postMessage({ type:'invertAlg', output: inv }); } } catch(e){ self.postMessage({ type:'error', detail:String(e) }); }
      self.addEventListener('message', ev => { if (ev.data && ev.data.cmd === 'ping') self.postMessage({ type: 'pong' }); });
    })();
  `;

  const blob = new Blob([script], { type: 'application/javascript' });
  const url = URL.createObjectURL(blob);
  const w = new Worker(url);
  // Provide a small wrapper API
  return {
    worker: w,
    postMessage: msg => w.postMessage(msg),
    onMessage: handler => w.addEventListener('message', ev => handler(ev.data)),
    terminate: () => { w.terminate(); URL.revokeObjectURL(url); }
  };
}

if (typeof module !== 'undefined' && module.exports) module.exports = { createWorkerFromTools };
