importScripts('solver.js');

self.onmessage = function (event) {
	const { scr, rot, num, len, restrict } = event.data;
	console.log(scr, rot, num, len, restrict);
	Module.onRuntimeInitialized = function () {
		try {
			Module.solve(scr, rot, num, len, restrict);
		} catch (e) {
			self.postMessage("Error");
		}
	};
};