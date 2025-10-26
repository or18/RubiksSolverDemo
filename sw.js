const CACHE_NAME = 'pwa-cash_v29';

const urlsToCache = [
	'index.html',
	'2x2x2.html',
	'cross_trainer.html',
	'documentation.html',
	'eocross_trainer.html',
	'pairing_trainer.html',
	'pseudo_pairing_trainer.html',
	'pseudo_xcross_trainer.html',
	'xcross_trainer.html',
	'jsonEditor.html',
	'algTrainer.html',
	'manifest.json',
	'icons/icon-512x512.png',
	'icons/icon-192x192.png',
	'src/2x2solver/solver.js',
	'src/2x2solver/solver.wasm',
	'src/2x2solver/worker.js',
	'src/2x2solverLite/solver.js',
	'src/2x2solverLite/solver.wasm',
	'src/2x2solverLite/worker.js',
	'src/2x2solverLite/worker_scrambler.js',
	'src/crossAnalyzer/analyzer.js',
	'src/crossAnalyzer/analyzer.wasm',
	'src/crossAnalyzer/worker_analyzer.js',
	'src/crossSolver/solver.js',
	'src/crossSolver/solver.wasm',
	'src/crossSolver/worker.js',
	'src/crossTrainer/solver.js',
	'src/crossTrainer/solver.wasm',
	'src/crossTrainer/worker.js',
	'src/EOCrossAnalyzer/analyzer.js',
	'src/EOCrossAnalyzer/analyzer.wasm',
	'src/EOCrossAnalyzer/worker_analyzer.js',
	'src/EOCrossSolver/solver.js',
	'src/EOCrossSolver/solver.wasm',
	'src/EOCrossSolver/worker.js',
	'src/eocrossTrainer/solver.js',
	'src/eocrossTrainer/solver.wasm',
	'src/eocrossTrainer/worker.js',
	'src/F2L_PairingSolver/pairing_solver.js',
	'src/F2L_PairingSolver/pairing_solver.wasm',
	'src/F2L_PairingSolver/worker_pairing.js',
	'src/functions/functions.js',
	'src/functions/functions.wasm',
	'src/functions/list.js',
	'src/highMemorySolver/solver2.js',
	'src/highMemorySolver/solver2.wasm',
	'src/highMemorySolver/worker2.js',
	'src/min2phase.js/min2phase.js',
	'src/pairAnalyzer/analyzer.js',
	'src/pairAnalyzer/analyzer.wasm',
	'src/pairAnalyzer/worker_analyzer.js',
	'src/pairingTrainer/solver.js',
	'src/pairingTrainer/solver.wasm',
	'src/pairingTrainer/worker.js',
	'src/pseudoCrossAnalyzer/pseudo_analyzer.js',
	'src/pseudoCrossAnalyzer/pseudo_analyzer.wasm',
	'src/pseudoCrossAnalyzer/worker_panalyzer.js',
	'src/pseudoCrossSolver/pseudo.js',
	'src/pseudoCrossSolver/pseudo.wasm',
	'src/pseudoCrossSolver/worker3.js',
	'src/pseudoPairAnalyzer/analyzer.js',
	'src/pseudoPairAnalyzer/analyzer.wasm',
	'src/pseudoPairAnalyzer/worker_analyzer.js',
	'src/pseudoPairingSolver/pseudoPairingSolver.js',
	'src/pseudoPairingSolver/pseudoPairingSolver.wasm',
	'src/pseudoPairingSolver/workerPseudoPairing.js',
	'src/pseudoPairingTrainer/solver.js',
	'src/pseudoPairingTrainer/solver.wasm',
	'src/pseudoPairingTrainer/worker.js',
	'src/pseudoXcrossTrainer/solver.js',
	'src/pseudoXcrossTrainer/solver.wasm',
	'src/pseudoXcrossTrainer/worker.js',
	'src/xcrossTrainer/solver.js',
	'src/xcrossTrainer/solver.wasm',
	'src/xcrossTrainer/worker.js',
];

self.addEventListener('install', (event) => {
	event.waitUntil(
		caches.open(CACHE_NAME)
			.then((cache) => cache.addAll(urlsToCache))
	);
});

self.addEventListener('activate', (event) => {
	const cacheWhitelist = [CACHE_NAME];
	event.waitUntil(
		caches.keys().then((cacheNames) => {
			return Promise.all(
				cacheNames.map((cacheName) => {
					if (cacheWhitelist.indexOf(cacheName) === -1) {
						return caches.delete(cacheName);
					}
				})
			);
		})
	);
});

self.addEventListener('fetch', (event) => {
	const { request } = event;
	if (request.mode === 'navigate') {
		event.respondWith(
			caches.match(request, { ignoreSearch: true })
				.then((response) => {
					return response || fetch(request);
				})
		);
		return;
	}
	event.respondWith(
		caches.match(request)
			.then((response) => {
				return response || fetch(request);
			})
	);
});