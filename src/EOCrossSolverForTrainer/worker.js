
const solverPromise = new Promise(resolve => {
	self.Module = {
		onRuntimeInitialized: () => resolve(self.Module)
	};
});

importScripts('solver.js');

self.onmessage = async function (event) {
	const { scr, rot, slot, num, len, restrict } = event.data;
	try {
		const Module = await solverPromise;
		Module.solve(scr, rot, slot, num, len, restrict);
	} catch (e) {
		self.postMessage("Error");
	}
};