// Worker that waits for an init message from main providing baseUrl
// Main should postMessage({ cmd: 'init', baseUrl: '...' }) before other commands
(function () {
  'use strict';
  function send(msg) { try { self.postMessage(msg); } catch (e) {} }

  let tools = null;
  self.addEventListener('message', async function (ev) {
    const d = ev.data || {};
    if (!d || !d.cmd) return;
    if (d.cmd === 'init') {
      try {
        importScripts('../utils/tools.js');
      } catch (e) {
        send({ type: 'error', error: 'importScripts_failed', detail: String(e) });
        return;
      }
      tools = (typeof self.__TOOLS_UTILS_EXPORTS__ !== 'undefined') ? self.__TOOLS_UTILS_EXPORTS__ : null;
      if (!tools) { send({ type: 'error', error: 'no_tools_exports' }); return; }
      try {
        send({ type: 'after_importscripts' });
        send({ type: 'tools_global_exists', exists: true });
        send({ type: 'init_started' });
        try {
          const p = tools.init({ baseUrl: d.baseUrl || '../utils/' });
          if (p && typeof p.then === 'function') {
            p.then(function(){ send({ type: 'init_done' }); }).catch(function(e){ send({ type: 'init_failed', detail: String(e) }); });
            await p.catch(function(){ /* handled above */ });
          } else {
            send({ type: 'init_done_sync' });
          }
        } catch (e) {
          send({ type: 'init_failed', detail: String(e) });
        }
        send({ type: 'init', mode: 'dynamic' });
      } catch (e) {
        send({ type: 'error', error: 'init_exception', detail: String(e) });
      }
    }
    if (d.cmd === 'ping') send({ type: 'pong' });
  });
})();
