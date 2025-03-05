importScripts('pairing_solver.js');

self.onmessage = function(event){
	const {scr, rot, slot, pslot, num, len, restrict} = event.data;
	Module.onRuntimeInitialized = function(){
		Module.solve(scr, rot, slot, pslot, num, len, restrict);
	};
};