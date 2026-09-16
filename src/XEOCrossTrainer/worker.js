let xeocrossSearchInstance;

const initPromise = new Promise((resolve, reject) => {
    self.Module = {
        onRuntimeInitialized: () => {
            try {
                xeocrossSearchInstance = new self.Module.xeocross_search();
                resolve();
            } catch (e) {
                reject("Initialization Error");
            }
        }
    };
});

importScripts('xeocross_solver_prod.js');

self.onmessage = async function (event) {
    const { scr, len } = event.data;
    try {
        await initPromise;
        if (xeocrossSearchInstance) {
            const ret = xeocrossSearchInstance.func(scr, len);
            self.postMessage(ret);
        }
    } catch (e) {
        self.postMessage("Error");
    }
};
