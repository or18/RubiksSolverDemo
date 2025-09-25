
const solverPromise = new Promise(resolve => {
	self.Module = {
		onRuntimeInitialized: () => resolve(self.Module)
	};
});

importScripts('solver.js');

self.onmessage = async function (event) {
	const { scr, rot, num, len, restrict, prune } = event.data;
	try {
		const Module = await solverPromise;
		Module.solve(scr, rot, num, len, restrict, prune);
	} catch (e) {
		self.postMessage("Error");
	}
};