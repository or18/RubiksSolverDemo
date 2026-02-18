// Entry script for Node worker_threads to provide similar behavior to web worker.js
const { parentPort } = require('worker_threads');
const path = require('path');

(async () => {
  try {
    // Load tools via require
    const tools = require(path.resolve(__dirname, '..', 'utils', 'tools.js'));

    // Attempt to init (may try to load wasm in Node environment)
    try {
      parentPort.postMessage({ type: 'init_started' });
      if (typeof tools.init === 'function') {
        try {
          await tools.init({ baseUrl: path.resolve(__dirname, '..', 'utils') + path.sep });
          parentPort.postMessage({ type: 'init_done' });
        } catch (e) {
          parentPort.postMessage({ type: 'init_failed', detail: String(e) });
        }
      }
    } catch (e) {
      // continue even if init fails
      parentPort.postMessage({ type: 'init_failed', detail: String(e) });
    }

    parentPort.postMessage({ type: 'init', hasScrFix: typeof tools.scr_fix === 'function', hasInvertAlg: typeof tools.invertAlg === 'function' });

    // Run small tests
    try {
      if (typeof tools.scr_fix === 'function') {
        const fixed = tools.scr_fix("R U R' U'");
        parentPort.postMessage({ type: 'scr_fix', input: "R U R' U'", output: fixed });
      }
      if (typeof tools.invertAlg === 'function') {
        const inv = await Promise.resolve(tools.invertAlg('R U R2'));
        parentPort.postMessage({ type: 'invertAlg', input: 'R U R2', output: inv });
      }
    } catch (e) {
      parentPort.postMessage({ type: 'error', error: 'test_failed', detail: String(e) });
    }

    parentPort.on('message', msg => {
      if (msg && msg.cmd === 'ping') parentPort.postMessage({ type: 'pong' });
    });
  } catch (e) {
    parentPort.postMessage({ type: 'fatal', detail: String(e) });
  }
})();
