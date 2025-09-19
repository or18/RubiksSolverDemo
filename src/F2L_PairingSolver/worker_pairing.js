importScripts('pairing_solver.js');

self.onmessage = function (event) {
	const { scr, rot, slot, pslot, num, len, move_restrict, post_alg, center_offset, max_rot_count, ma2, mcString } = event.data;
	Module.onRuntimeInitialized = function () {
		try {
			Module.solve(scr, rot, slot, pslot, num, len, move_restrict, post_alg, center_offset, max_rot_count, ma2, mcString);
		} catch (e) {
			self.postMessage("Error");
		}
	};
};