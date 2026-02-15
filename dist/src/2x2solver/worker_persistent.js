/**
 * 2x2 Persistent Solver Worker - Web Worker wrapper
 * 
 * Uses MODULARIZE version (solver.js) for universal compatibility.
 * Same binary used in Node.js and Web Worker.
 * Works with both local files and CDN loading.
 * 
 * Message format (to worker):
 *   {
 *     scramble: string (required)
 *     rotation: string (default: "")
 *     maxSolutions: number (default: 3)
 *     maxLength: number (default: 11)
 *     pruneDepth: number (default: 1)
 *     allowedMoves: string (default: 'U_U2_U-_R_R2_R-_F_F2_F-')
 *     preMove: string (default: '')
 *     moveOrder: string (default: '')
 *     moveCount: string (default: '')
 *   }
 * 
 * Response format (postMessage):
 *   { type: 'solution', data: 'solution string' }
 *   { type: 'depth', data: 'depth=N' }
 *   { type: 'done', data: null }
 *   { type: 'error', data: 'error message' }
 *   { type: 'ready', data: null }
 */

let persistentSolver = null;

// CRITICAL: Save original postMessage before overwriting
const originalPostMessage = self.postMessage.bind(self);

// Intercept postMessage from C++ and convert to structured format
globalThis.postMessage = function(message) {
	if (typeof message === 'string') {
		if (message === 'Search finished.') {
			originalPostMessage({
				type: 'done',
				data: null
			});
		} else if (message.startsWith('Error')) {
			originalPostMessage({
				type: 'error',
				data: message
			});
		} else if (message === 'Already solved.') {
			originalPostMessage({
				type: 'solution',
				data: ''
			});
			originalPostMessage({
				type: 'done',
				data: null
			});
		} else if (message.startsWith('depth=')) {
			// Send depth progress updates
			originalPostMessage({
				type: 'depth',
				data: message
			});
		} else {
			// Normal solution string
			originalPostMessage({
				type: 'solution',
				data: message
			});
		}
	}
};

// Determine base path for assets (solver.js, solver.wasm)
// This works for both local files and CDN
const scriptPath = self.location.href;
const baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);

// Import MODULARIZE version (same as Node.js)
importScripts(baseURL + 'solver.js');

// Initialize with createModule() factory
// Use locateFile to ensure WASM is loaded from the correct path
createModule({
	locateFile: function(path) {
		// Ensure WASM and other assets are loaded from the same directory as worker
		if (path.endsWith('.wasm') || path.endsWith('.wasm.map')) {
			return baseURL + path;
		}
		return path;
	}
}).then(Module => {
	try {
		persistentSolver = new Module.PersistentSolver2x2();
		
		originalPostMessage({
			type: 'ready',
			data: null
		});
	} catch (error) {
		originalPostMessage({
			type: 'error',
			data: 'Failed to create PersistentSolver2x2: ' + error.message
		});
	}
}).catch(error => {
	originalPostMessage({
		type: 'error',
		data: 'Failed to initialize module: ' + error.message
	});
});

self.onmessage = async function(event) {
	try {
		if (!persistentSolver) {
			originalPostMessage({
				type: 'error',
				data: 'Solver not initialized yet'
			});
			return;
		}
		
		const data = event.data;
		
		// Extract parameters with defaults
		const scramble = data.scramble || data.scr || '';
		const rotation = data.rotation !== undefined ? data.rotation : (data.rot !== undefined ? data.rot : '');
		const maxSolutions = data.maxSolutions !== undefined ? data.maxSolutions : (data.num !== undefined ? data.num : 3);
		const maxLength = data.maxLength !== undefined ? data.maxLength : (data.len !== undefined ? data.len : 11);
		const pruneDepth = data.pruneDepth !== undefined ? data.pruneDepth : (data.prune !== undefined ? data.prune : 1);
		const allowedMoves = data.allowedMoves || data.rest || data.move_restrict || 'U_U2_U-_R_R2_R-_F_F2_F-';
		const preMove = data.preMove || data.premove || data.post_alg || '';
		const moveOrder = data.moveOrder || data.mav || data.ma2 || '';
		const moveCount = data.moveCount || data.mcv || data.mcString || '';
		
		// Input validation
		if (!scramble) {
			originalPostMessage({
				type: 'error',
				data: 'scramble parameter is required'
			});
			return;
		}
		
		if (typeof maxSolutions !== 'number' || maxSolutions < 1) {
			originalPostMessage({
				type: 'error',
				data: 'maxSolutions must be a number >= 1'
			});
			return;
		}
		
		if (typeof maxLength !== 'number' || maxLength < 1 || maxLength > 30) {
			originalPostMessage({
				type: 'error',
				data: 'maxLength must be a number between 1 and 30'
			});
			return;
		}
		
		if (typeof pruneDepth !== 'number' || pruneDepth < 0 || pruneDepth > 20) {
			originalPostMessage({
				type: 'error',
				data: 'pruneDepth must be a number between 0 and 20'
			});
			return;
		}
		
		// Basic scramble format validation (space-separated moves)
		const movePattern = /^[URFLBD][2']?$/;
		const moves = scramble.trim().split(/\s+/).filter(m => m);
		for (const move of moves) {
			if (!movePattern.test(move)) {
				originalPostMessage({
					type: 'error',
					data: `Invalid move format: "${move}". Use format like: R U R' U2 F2`
				});
				return;
			}
		}
		
		// Call persistent solver (reuses prune table)
		persistentSolver.solve(
			scramble,
			rotation,
			maxSolutions,
			maxLength,
			pruneDepth,
			allowedMoves,
			preMove,
			moveOrder,
			moveCount
		);
		
	} catch (e) {
		console.error('Worker error:', e);
		originalPostMessage({
			type: 'error',
			data: e.message || 'Unknown error occurred'
		});
	}
};
