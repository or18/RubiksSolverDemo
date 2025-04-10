importScripts('solver2.js');

self.onmessage = function (event) {
	const { solver, scr, rot, slot, ll, num, len, restrict } = event.data;
	Module.onRuntimeInitialized = function () {
		try {
			Module.solve(solver, scr, rot, slot, ll, num, len, restrict);
		} catch (e) {
			self.postMessage("Error");
		}
	};
};