importScripts('f2leo_xcross_solver.js');

let f2leoXCrossSearchInstance = null;

const initPromise = new Promise((resolve, reject) => {
    createF2LEOXCrossModule()
        .then((Module) => {
            try {
                f2leoXCrossSearchInstance = new Module.f2leo_xcross_search();
                resolve();
            } catch (e) {
                console.error("Failed to instantiate f2leo_xcross_search:", e);
                reject(e);
            }
        })
        .catch((err) => {
            console.error("Failed to load createF2LEOXCrossModule:", err);
            reject(err);
        });
});

self.onmessage = async function (event) {
    const { scr, len } = event.data;
    try {
        await initPromise;
        if (f2leoXCrossSearchInstance) {
            const ret = f2leoXCrossSearchInstance.func(scr || "", String(len));
            self.postMessage(ret);
        } else {
            self.postMessage("Error: Instance not ready");
        }
    } catch (e) {
        console.error("Worker message handling error:", e);
        self.postMessage("Error");
    }
};
