// Minimal persistent-style worker that initializes tools.js via importScripts
/* This worker is a test harness. It expects to be served from dist/ where
   fixtures/tools.js, functions.js and functions.wasm are available under
   appropriate paths. It will attempt to load tools via importScripts and
   exercise a couple of functions (scr_fix and invertAlg) and post results
   back to the main thread. */

(function () {
  'use strict';

  function send(msg) {
    try {
      self.postMessage(msg);
    } catch (e) {
      // ignore
    }
  }

  // Try to load tools via importScripts (non-module worker)
  try {
    // Load the real tools bundle from repo layout. From this worker's
    // location (dist/src/test_worker/worker.js) the utils are at ../utils/tools.js
    importScripts('../utils/tools.js');
  } catch (e) {
    send({ type: 'error', error: 'importScripts_failed', detail: String(e) });
  }

  // Obtain the exports. tools.js should expose __TOOLS_UTILS_EXPORTS__ as global.
  var tools = (typeof self.__TOOLS_UTILS_EXPORTS__ !== 'undefined') ? self.__TOOLS_UTILS_EXPORTS__ : null;

  if (!tools) {
    send({ type: 'error', error: 'no_tools_exports' });
    return;
  }

  // Ensure the C++ module base is set correctly for this worker.
  // tools.init accepts `{ baseUrl }` which will be used when loading
  // cpp-functions/functions.js via importScripts in a worker.
  Promise.resolve().then(() => {
    return tools.init({ baseUrl: '../utils/' }).catch(err => {
      // init may fail if C++ artifacts are absent; report but continue
      send({ type: 'init_failed', detail: String(err) });
      return null;
    });
  }).then(() => {
    // Basic capability checks
    var hasScrFix = typeof tools.scr_fix === 'function';
    var hasInvert = typeof tools.invertAlg === 'function';

    send({ type: 'init', hasScrFix: hasScrFix, hasInvertAlg: hasInvert });

    // Run small tests
    try {
      if (hasScrFix) {
        var fixed = tools.scr_fix("R U R' U'");
        send({ type: 'scr_fix', input: "R U R' U'", output: fixed });
      }
      if (hasInvert) {
        // invertAlg may be async depending on implementation
        Promise.resolve(tools.invertAlg("R U R2")).then(function (inv) {
          send({ type: 'invertAlg', input: "R U R2", output: inv });
        }).catch(function (e) {
          send({ type: 'error', error: 'invert_failed', detail: String(e) });
        });
      }
    } catch (e) {
      send({ type: 'error', error: 'test_failed', detail: String(e) });
    }
  });

  // Respond to pings from main thread
  self.addEventListener('message', function (ev) {
    var d = ev.data || {};
    if (d && d.cmd === 'ping') {
      send({ type: 'pong' });
    }
  });
})();
