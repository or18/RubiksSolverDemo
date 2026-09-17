let searchInstance = null;
let Module = null;

importScripts('xxxcross_solver_prod.js');

const modulePromise = (async () => {
    try {
        console.log('[Worker] Initializing WASM module (calling createXXXCrossModule)...');
        Module = await self.createXXXCrossModule();
        console.log('[Worker] Constructing xxxcross_search single-instance...');
        searchInstance = new Module.xxxcross_search();
        console.log('[Worker] xxxcross_search ready.');
        return searchInstance;
    } catch (e) {
        console.error('[Worker] Initialization Error:', e);
        throw e;
    }
})();

self.onmessage = async function (event) {
    const { scr, len, slot } = event.data;
    try {
        await modulePromise;
        if (!searchInstance) {
            self.postMessage("Initial Error");
            return;
        }

        const slotParam = slot || "FL";
        const lenParam = (len !== undefined && len !== null) ? len.toString() : "7";

        // C++: func(arg_scramble, arg_length, arg_slot)
        const ret = searchInstance.func(scr, lenParam, slotParam);
        self.postMessage(ret);
    } catch (e) {
        console.error('[Worker] Search execution error:', e);
        self.postMessage("Error");
    }
};
