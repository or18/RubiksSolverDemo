importScripts('solver.js');

self.onmessage = function (event) {
	const { scr, rot, num, len, restrict, prune } = event.data;
	console.log(scr, rot, num, len, restrict, prune);
	Module.onRuntimeInitialized = function () {
		try {
			Module.solve(scr, rot, num, len, restrict, prune);
		} catch (e) {
			self.postMessage("Error");
		}
	};
};