
const solverPromise = new Promise(resolve => {
	self.Module = {
		onRuntimeInitialized: () => resolve(self.Module)
	};
});

importScripts('solver.js');

self.onmessage = async function (event) {
	const { solver, scr, rot, slot, ll, num, len, restrict } = event.data;
	try {
		const Module = await solverPromise;
		Module.solve(solver, scr, rot, slot, ll, num, len, restrict);
	} catch (e) {
		self.postMessage("Error");
	}
};