// XXcross Solver Worker - Dual Instance Support (Adjacent + Opposite)
// 2026-01-04: Production WASM module for xxcross_trainer.html

let xxcrossSearchInstance_adjacent = null;
let xxcrossSearchInstance_opposite = null;
let Module = null; // Will hold the initialized WASM module

// Helper function to send log messages to main thread
function workerLog(message, type = 'info') {
	console.log(message);
	self.postMessage({ log: message, logType: type });
}

// Module initialization
workerLog('[Worker] Loading solver_prod.js...', 'info');
importScripts('solver_prod.js');

// Initialize WASM module (MODULARIZE=1 exports createModule function)
const modulePromise = (async () => {
	workerLog('[Worker] Initializing WASM module (calling createModule)...', 'info');
	try {
		// createModule is defined by solver_prod.js (MODULARIZE=1)
		Module = await self.createModule();
		workerLog('[Worker] WASM module initialized successfully', 'success');
		return Module;
	} catch (error) {
		workerLog(`[Worker] WASM module initialization failed: ${error.message}`, 'error');
		throw error;
	}
})();

self.onmessage = async function (event) {
	const { scr, len, pairType, bucketModel } = event.data;
	
	workerLog(`[Worker] Received message: pairType=${pairType}, bucketModel=${bucketModel}, len=${len}, scr="${scr}"`, 'info');
	
	try {
		// Wait for module initialization
		workerLog('[Worker] Waiting for module initialization...', 'info');
		await modulePromise;
		await modulePromise;
		workerLog('[Worker] Module ready', 'success');
		
		// Validate pairType
		if (pairType !== 'adj' && pairType !== 'opp') {
			const error = `Invalid pairType: ${pairType}. Expected 'adj' or 'opp'.`;
			workerLog(`[Worker] ${error}`, 'error');
			self.postMessage({ error });
			return;
		}
		
		// Lazy initialization: Create instance only when needed
		if (pairType === 'adj' && !xxcrossSearchInstance_adjacent) {
			const adj = true;
			const model = bucketModel || "DESKTOP_STD";
			workerLog(`[Worker] Creating adjacent solver instance (model: ${model})...`, 'info');
			workerLog('[Worker] Database construction starting... (this may take 10-80 seconds)', 'warning');
			const startTime = performance.now();
			xxcrossSearchInstance_adjacent = new Module.xxcross_search(adj, model);
			const elapsed = ((performance.now() - startTime) / 1000).toFixed(1);
			workerLog(`[Worker] Adjacent solver initialized in ${elapsed}s`, 'success');
		} else if (pairType === 'opp' && !xxcrossSearchInstance_opposite) {
			const adj = false;
			const model = bucketModel || "DESKTOP_STD";
			workerLog(`[Worker] Creating opposite solver instance (model: ${model})...`, 'info');
			workerLog('[Worker] Database construction starting... (this may take 10-80 seconds)', 'warning');
			const startTime = performance.now();
			xxcrossSearchInstance_opposite = new Module.xxcross_search(adj, model);
			const elapsed = ((performance.now() - startTime) / 1000).toFixed(1);
			workerLog(`[Worker] Opposite solver initialized in ${elapsed}s`, 'success');
		}
		
		// Select appropriate instance
		const instance = pairType === 'adj' 
			? xxcrossSearchInstance_adjacent 
			: xxcrossSearchInstance_opposite;
		
		// Execute solver
		if (instance) {
			workerLog(`[Worker] Calling func(scr="${scr}", len=${len})...`, 'info');
			const funcStartTime = performance.now();
			const ret = instance.func(scr, len);
			const funcElapsed = (performance.now() - funcStartTime).toFixed(2);
			workerLog(`[Worker] func() returned in ${funcElapsed}ms: ${ret.substring(0, 100)}${ret.length > 100 ? '...' : ''}`, 'success');
			self.postMessage({ result: ret, pairType });
		} else {
			const error = "Instance creation failed";
			workerLog(`[Worker] ${error}`, 'error');
			self.postMessage({ error });
		}
	} catch (e) {
		const error = `Worker error: ${e.message || e}`;
		workerLog(`[Worker] ${error}`, 'error');
		console.error(e);
		self.postMessage({ error });
	}
};

workerLog('[Worker] Message handler registered, ready to receive messages', 'success');
