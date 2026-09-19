importScripts('f2leo_xxcross_solver.js');

let f2leoXXCrossSearchInstance = null;

const initPromise = new Promise((resolve, reject) => {
    createF2LEOXXCrossModule()
        .then((Module) => {
            try {
                f2leoXXCrossSearchInstance = new Module.f2leo_xxcross_search();
                resolve();
            } catch (e) {
                console.error("Failed to instantiate f2leo_xxcross_search:", e);
                reject(e);
            }
        })
        .catch((err) => {
            console.error("Failed to load createF2LEOXXCrossModule:", err);
            reject(err);
        });
});

self.onmessage = async function (event) {
    const { scr, len, slot } = event.data;
    try {
        await initPromise;
        if (f2leoXXCrossSearchInstance) {
            const ret = f2leoXXCrossSearchInstance.func(scr || "", String(len), slot || "BL BR");
            self.postMessage(ret);
        } else {
            self.postMessage("Error: Instance not ready");
        }
    } catch (e) {
        console.error("Worker message handling error:", e);
        self.postMessage("Error");
    }
};
