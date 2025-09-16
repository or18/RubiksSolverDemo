importScripts('pseudo.js');

self.onmessage = function (event) {
	const { scr, rot, slot, pslot, num, len, move_restrict, post_alg, center_offset, max_rot_count, ma2 } = event.data;
	Module.onRuntimeInitialized = function () {
		try {
			Module.solve(scr, rot, slot, pslot, num, len, move_restrict, post_alg, center_offset, max_rot_count, ma2);
		} catch (e) {
			self.postMessage("Error");
		}
	};
};