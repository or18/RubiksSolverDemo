importScripts('f2leo_solver.js');

let f2leoSearchInstance = null;

const initPromise = new Promise((resolve, reject) => {
    createF2LEOModule()
        .then((Module) => {
            try {
                f2leoSearchInstance = new Module.f2leo_search();
                resolve();
            } catch (e) {
                console.error("Failed to instantiate f2leo_search:", e);
                reject(e);
            }
        })
        .catch((err) => {
            console.error("Failed to load createF2LEOModule:", err);
            reject(err);
        });
});

self.onmessage = async function (event) {
    const { scr, len } = event.data;
    try {
        await initPromise;
        if (f2leoSearchInstance) {
            const ret = f2leoSearchInstance.func(scr || "", String(len));
            self.postMessage(ret);
        } else {
            self.postMessage("Error: Instance not ready");
        }
    } catch (e) {
        console.error("Worker message handling error:", e);
        self.postMessage("Error");
    }
};
