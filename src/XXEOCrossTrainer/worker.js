let xxeocrossSearchInstance;

importScripts('xxeocross_solver_prod.js');

const initPromise = createXXEOCrossModule().then((Module) => {
    xxeocrossSearchInstance = new Module.xxeocross_search();
}).catch((err) => {
    console.error("Initialization Error:", err);
    throw err;
});

self.onmessage = async function (event) {
    const { scr, len, slot } = event.data;
    try {
        await initPromise;
        if (xxeocrossSearchInstance) {
            const ret = xxeocrossSearchInstance.func(scr || "", String(len || "7"), slot || "BL BR");
            self.postMessage(ret);
        } else {
            self.postMessage("Error");
        }
    } catch (e) {
        self.postMessage("Error");
    }
};
