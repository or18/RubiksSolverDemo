const { Worker } = require('worker_threads');
const path = require('path');

function startNodeWorker(options = {}) {
  const entry = path.resolve(__dirname, 'worker-node-entry.js');
  const worker = new Worker(entry, { argv: [] });

  const listeners = new Map();

  function onMessage(fn) {
    worker.on('message', fn);
    listeners.set(fn, fn);
  }

  function removeMessage(fn) {
    worker.off('message', fn);
    listeners.delete(fn);
  }

  function postMessage(msg) {
    worker.postMessage(msg);
  }

  function terminate() {
    return worker.terminate();
  }

  return { worker, onMessage, removeMessage, postMessage, terminate };
}

if (require.main === module) {
  // If invoked directly, run a small demo
  const w = startNodeWorker();
  w.onMessage(msg => console.log('WORKER:', msg));
  setTimeout(() => w.postMessage({ cmd: 'ping' }), 1000);
}

module.exports = { startNodeWorker };
