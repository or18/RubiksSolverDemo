importScripts('solver.js');

let xcrossSearchInstance;
let initPromise = new Promise((resolve, reject) => {
	Module.onRuntimeInitialized = function () {
		if (!xcrossSearchInstance) {
			try {
				xcrossSearchInstance = new Module.xcross_search();
				resolve();
			} catch (e) {
				reject("Error");
			}
		}
	};
});

self.onmessage = async function (event) {
	const { scr, len } = event.data;
	try {
		await initPromise;
		if (xcrossSearchInstance) {
			const ret = xcrossSearchInstance.func(scr, len);
			self.postMessage(ret);
		}
	} catch (e) {
		self.postMessage("Error");
	}
};

