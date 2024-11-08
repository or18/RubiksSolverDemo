importScripts('pseudo_analyzer.js');

self.onmessage = function(event){
	const {scramble, cross, x, xx, xxx} = event.data;
	Module.onRuntimeInitialized = function(){
		Module.analyze(scramble, cross, x, xx, xxx);
	};
};