/**
 * 2x2 Solver Worker - Hybrid compatible version
 * 
 * C++ function signature:
 *   solve(scramble, rotation, num, len, prune, move_restrict, post_alg, ma2, mcString)
 * 
 * Parameters (2x2x2.html compliant):
 *   scramble / scr: Scramble string (required)
 *   rotation / rot: Whole-cube rotation ("", "y", "z", "z2", etc., default: "")
 *   maxSolutions / num: Maximum number of solutions (default: 20)
 *   maxLength / len: Maximum solution length (default: 20)
 *   pruneDepth / prune: Pruning depth (default: 8)
 *   allowedMoves / rest / move_restrict: Allowed move set (default: 'U_U2_U-_R_R2_R-_F_F2_F-')
 *   preMove / premove / post_alg: Pre-move sequence (default: '')
 *   moveOrder / mav / ma2: Move order constraint (default: '')
 *   moveCount / mcv / mcString: Move count constraint (default: '')
 * 
 * Response format (postMessage):
 *   { type: 'solution', data: 'solution' }
 *   { type: 'done', data: null }
 *   { type: 'error', data: 'error message' }
 */

// Intercept postMessage from C++ and convert to new format
const originalPostMessage = self.postMessage.bind(self);
self.postMessage = function(message) {
	// Convert raw string messages from C++ to new format
	if (typeof message === 'string') {
		if (message === 'Search finished.' || message === 'search_finished') {
			originalPostMessage({
				type: 'done',
				data: null
			});
		} else if (message.startsWith('Error') || message === 'error') {
			originalPostMessage({
				type: 'error',
				data: message
			});
		} else if (message === 'Already solved.') {
			// Already solved is treated as an empty solution
			originalPostMessage({
				type: 'solution',
				data: ''
			});
			originalPostMessage({
				type: 'done',
				data: null
			});
		} else if (message.startsWith('depth=') || message.startsWith('Depth=')) {
			// Ignore debug messages from C++ solver
			// These are progress indicators, not solutions
			return;
		} else {
			// Normal solution string from C++ is sent as a solution message
			originalPostMessage({
				type: 'solution',
				data: message
			});
		}
	} else {
		// If it's already a new message format, send it as is.
		originalPostMessage(message);
	}
};

const solverPromise = new Promise(resolve => {
	self.Module = {
		onRuntimeInitialized: () => resolve(self.Module)
	};
});

importScripts('solver.js');

self.onmessage = async function (event) {
	try {
		const data = event.data;
		
		// Parameter Compatibility Handling (Compliant with 2x2x2.html)
		
		// scramble: Scrambled string (required)
		const scramble = data.scramble || data.scr || '';
		
		// rotation: Whole rotation premove ("", "y", "z", "z2", etc., default: "")
		// rotation: Whole-cube rotation ("", "y", "z", "z2", etc., default: "")
		const rotation = data.rotation !== undefined ? data.rotation : 
		                 (data.rot !== undefined ? data.rot : '');
		
		// num: Maximum number of solutions (default: 20)
		const num = data.maxSolutions !== undefined ? data.maxSolutions : 
		            (data.num !== undefined ? data.num : 20);
		
		// len: Maximum solution length (default: 20)
		const len = data.maxLength !== undefined ? data.maxLength : 
		            (data.len !== undefined ? data.len : 20);
		
		// prune: Pruning depth (default: 8)
		const prune = data.pruneDepth !== undefined ? data.pruneDepth : 
		              (data.prune !== undefined ? data.prune : 8);
		
		// move_restrict: Allowed move set (default: URF all moves)
		const moveRestrict = data.allowedMoves || data.rest || data.move_restrict || 'U_U2_U-_R_R2_R-_F_F2_F-';
		
		// post_alg: Pre-move sequence (default: '')
		const postAlg = data.preMove || data.premove || data.post_alg || '';
		
		// ma2: Move order constraint (default: '')
		const ma2 = data.moveOrder || data.mav || data.ma2 || '';
		
		// mcString: Move count limit (default: '')
		const mcString = data.moveCount || data.mcv || data.mcString || '';
		
		// Returns an error if the scramble is empty.
		if (!scramble) {
			originalPostMessage({
				type: 'error',
				data: 'scramble parameter is required'
			});
			return;
		}
		
		// Run the solver
		// C++ function: solve(scramble, rotation, num, len, prune, move_restrict, post_alg, ma2, mcString)
		const Module = await solverPromise;
		Module.solve(scramble, rotation, num, len, prune, moveRestrict, postAlg, ma2, mcString);
		
	} catch (e) {
		console.error('Worker error:', e);
		originalPostMessage({
			type: 'error',
			data: e.message || 'Unknown error occurred'
		});
	}
};

