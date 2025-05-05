importScripts('solver.js');

let crossSearchInstance;
let initPromise = new Promise((resolve, reject) => {
    Module.onRuntimeInitialized = function () {
        if (!crossSearchInstance) {
            try {
                crossSearchInstance = new Module.cross_search();
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
        if (crossSearchInstance) {
            const ret = crossSearchInstance.func(scr, len);
            self.postMessage(ret);
        }
    } catch (e) {
        self.postMessage("Error");
    }
};

