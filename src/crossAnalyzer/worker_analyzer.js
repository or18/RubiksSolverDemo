importScripts('analyzer.js');

self.onmessage = function(event){
	const {scramble, cross, x, xx, xxx, n} = event.data;
	Module.onRuntimeInitialized = function(){
		Module.analyze(scramble, cross, x, xx, xxx, n);
	};
};