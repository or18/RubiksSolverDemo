importScripts('pseudoPairingSolver.js');

self.onmessage = function (event) {
	const { scr, rot, slot, pslot, a_slot, a_pslot, num, len, restrict } = event.data;
	Module.onRuntimeInitialized = function () {
		try {
			Module.solve(scr, rot, slot, pslot, a_slot, a_pslot, num, len, restrict);
		} catch (e) {
			self.postMessage("Error");
		}
	};
};