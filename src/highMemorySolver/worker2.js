importScripts('solver2.js');

self.onmessage = function (event) {
	const { solver, scr, rot, slot, ll, num, len, move_restrict, post_alg, center_offset, max_rot_count, ma2, mcString } = event.data;
	Module.onRuntimeInitialized = function () {
		try {
			Module.solve(solver, scr, rot, slot, ll, num, len, move_restrict, post_alg, center_offset, max_rot_count, ma2, mcString);
		} catch (e) {
			self.postMessage("Error");
		}
	};
};