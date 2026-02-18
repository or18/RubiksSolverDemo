// Module-style worker entry that uses dynamic import() of tools.js
// Falls back to global __TOOLS_UTILS_EXPORTS__ when import() returns empty.
;(async () => {
  'use strict';
  function send(msg) { try { self.postMessage(msg); } catch (e) {} }

  let mod = null;
  try {
    mod = await import('../utils/tools.js');
  } catch (e) {
    send({ type: 'import_failed', detail: String(e) });
  }

  const tools = (mod && Object.keys(mod).length) ? mod : (self.__TOOLS_UTILS_EXPORTS__ || null);
  if (!tools) {
    send({ type: 'error', error: 'no_tools_exports' });
    return;
  }

  try {
    await tools.init({ baseUrl: '../utils/' }).catch(e => { send({ type: 'init_failed', detail: String(e) }); });
  } catch (e) {
    send({ type: 'init_failed', detail: String(e) });
  }

  send({ type: 'init', mode: 'module', hasScrFix: typeof tools.scr_fix === 'function', hasInvertAlg: typeof tools.invertAlg === 'function' });

  try {
    if (typeof tools.scr_fix === 'function') {
      send({ type: 'scr_fix', output: tools.scr_fix("R U R' U'") });
    }
    if (typeof tools.invertAlg === 'function') {
      const inv = await tools.invertAlg('R U R2');
      send({ type: 'invertAlg', output: inv });
    }
  } catch (e) {
    send({ type: 'error', error: 'test_failed', detail: String(e) });
  }

  self.addEventListener('message', ev => {
    const d = ev.data || {};
    if (d && d.cmd === 'ping') send({ type: 'pong' });
  });
})();
