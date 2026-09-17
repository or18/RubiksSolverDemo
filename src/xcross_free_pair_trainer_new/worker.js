let searchInstance = null;
let Module = null;

importScripts('xcross_free_pair_solver_prod.js');

const modulePromise = (async () => {
    try {
        console.log('[Worker] Initializing WASM module (calling createXCrossFreePairModule)...');
        Module = await self.createXCrossFreePairModule();
        console.log('[Worker] Constructing xxcross_search single-instance...');
        searchInstance = new Module.xxcross_search();
        console.log('[Worker] xxcross_search (Free Pair) ready.');
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

        const slotParam = slot || "BL BR";
        const lenParam = (len !== undefined && len !== null) ? len.toString() : "7";

        // C++: func(arg_scramble, arg_length, arg_slot)
        const ret = searchInstance.func(scr, lenParam, slotParam);
        self.postMessage(ret);
    } catch (e) {
        console.error('[Worker] Search execution error:', e);
        self.postMessage("Error");
    }
};
