
const solverPromise = new Promise(resolve => {
	self.Module = {
		onRuntimeInitialized: () => resolve(self.Module)
	};
});

importScripts('pseudoPairingSolver.js');

self.onmessage = async function (event) {
	const { scr, rot, slot, pslot, a_slot, a_pslot, num, len, restrict } = event.data;
	try {
		const Module = await solverPromise;
		Module.solve(scr, rot, slot, pslot, a_slot, a_pslot, num, len, restrict);
	} catch (e) {
		self.postMessage("Error");
	}
};