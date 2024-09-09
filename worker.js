importScripts('solver.js');

self.onmessage = function(event){
	const {solver, scr, rot, slot, ll, num, len, restrict} = event.data;
	Module.onRuntimeInitialized = function(){
		Module.solve(solver, scr, rot, slot, ll, num, len, restrict);
	};
};