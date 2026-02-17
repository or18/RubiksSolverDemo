// Auto-detect base URL for various environments (browser, worker, ESM, Node)
function detectBaseUrl() {
	// ESM / bundler: access import.meta.url via eval to avoid parse-time syntax errors
	try {
		const im = (typeof eval === 'function') ? eval('import.meta') : null;
		if (im && im.url) {
			return new URL('.', im.url).href;
		}
	} catch (e) { /* import.meta not available */ }

	// Browser main thread
	if (typeof document !== 'undefined') {
		// Prefer document.currentScript when available
		let cur = document.currentScript;
		if (cur && cur.src) {
			return cur.src.replace(/\/[^\/]*$/, '/');
		}
		// Try to locate a script element that points to this utils bundle (e.g. CDN URL)
		try {
			const scripts = document.getElementsByTagName('script');
			for (let i = scripts.length - 1; i >= 0; i--) {
				const s = scripts[i];
				if (!s || !s.src) continue;
				const srcAttr = s.getAttribute && s.getAttribute('src');
				if (!srcAttr) continue;
				if (srcAttr.endsWith('/tools.js') || /dist\/src\/utils/.test(srcAttr) || /\/tools\.js($|[?#])/.test(srcAttr)) {
					return s.src.replace(/\/[^\/]*$/, '/');
				}
			}
			// fallback: last script element with src
			const last = (document.scripts && document.scripts[document.scripts.length - 1]);
			if (last && last.src) return last.src.replace(/\/[^\/]*$/, '/');
		} catch (e) { /* ignore DOM access errors */ }
		// fallback to location
		if (typeof location !== 'undefined' && location.href) {
			return location.href.replace(/\/[^\/]*$/, '/');
		}
	}

	// Web Worker
	if (typeof self !== 'undefined' && self.location && self.location.href) {
		return self.location.href.replace(/\/[^\/]*$/, '/');
	}

	// Node
	if (typeof process !== 'undefined') {
		try {
			const path = require('path');
			return path.dirname(require.main && require.main.filename ? require.main.filename : __filename) + '/';
		} catch (e) { }
	}

	return './';
}

let _cppFunctionModulePromise = null;
let _initBase = null;

function _normalizeBase(b) {
	if (!b) return b;
	return b.endsWith('/') ? b : (b + '/');
}

async function _loadCppFunctionModule(baseUrl) {
	if (_cppFunctionModulePromise) return _cppFunctionModulePromise;

	const resolvedBase = (baseUrl !== undefined && baseUrl !== null) ? baseUrl : (_initBase || detectBaseUrl());
	const normalizedBase = _normalizeBase(resolvedBase);
	const scriptUrl = normalizedBase + 'cpp-functions/functions.js';

	_cppFunctionModulePromise = new Promise((resolve, reject) => {
		// Browser
		if (typeof window !== 'undefined') {
			try {
				const s = document.createElement('script');
				s.src = scriptUrl;
				s.onload = () => {
					// Prefer MODULARIZE=1 factory
					if (typeof createCppFunctionsModule === 'function') {
						try {
							const factory = createCppFunctionsModule;
							const opts = {
								locateFile: function (path) {
									return normalizedBase + 'cpp-functions/' + path;
								}
							};
							const maybePromise = factory(opts);
							Promise.resolve(maybePromise).then(m => resolve(m)).catch(reject);
						} catch (e) {
							reject(e);
						}
						return;
					}
					// Fallback to global Module
					if (window.Module) {
						const mod = window.Module;
						if (mod.calledRun) return resolve(mod);
						if (typeof mod.onRuntimeInitialized === 'function') {
							const orig = mod.onRuntimeInitialized;
							mod.onRuntimeInitialized = function() {
								try { orig(); } catch (e) { /* ignore */ }
								resolve(mod);
							};
							return;
						}
						// fallback: poll until the module runtime is initialized or until timeout
						const start = Date.now();
						const poll = setInterval(() => {
							if (mod.calledRun || typeof mod.cwrap === 'function') {
								clearInterval(poll);
								resolve(mod);
								return;
							}
							if (Date.now() - start > 10000) { // 10s timeout
								clearInterval(poll);
								// resolve anyway; caller can handle missing symbols
								resolve(mod);
							}
						}, 50);
						return;
					}
					return reject(new Error('Emscripten module not found after loading ' + scriptUrl));
				};
				s.onerror = (e) => reject(new Error('Failed to load ' + scriptUrl + ': ' + e));
				document.head.appendChild(s);
			} catch (e) {
				reject(e);
			}
			// Node
		} else if (typeof process !== 'undefined') {
			try {
				const path = require('path');
				const resolved = path.resolve(__dirname, baseUrl || '', 'cpp-functions/functions.js');
				const modFactory = require(resolved);
				if (typeof modFactory === 'function') {
					Promise.resolve(modFactory()).then(m => resolve(m)).catch(reject);
				} else {
					// modFactory is the Module object; wait until runtime is initialized
					const mod = modFactory;
					if (mod.calledRun) {
						return resolve(mod);
					}
					if (typeof mod.onRuntimeInitialized === 'function') {
						const orig = mod.onRuntimeInitialized;
						mod.onRuntimeInitialized = function() {
							try { orig(); } catch (e) { /* ignore */ }
							resolve(mod);
						};
						return;
					}
					// fallback: poll for calledRun
					const poll = setInterval(() => {
						if (mod.calledRun) {
							clearInterval(poll);
							resolve(mod);
						}
					}, 50);
				}
			} catch (e) {
				reject(e);
			}
		} else {
			reject(new Error('Unsupported environment for loading ' + scriptUrl));
		}
	});

	return _cppFunctionModulePromise;
}

// Public init: optional options.baseUrl
async function init(options = {}) {
	if (options.baseUrl) _initBase = _normalizeBase(options.baseUrl);
	return await _loadCppFunctionModule(_initBase);
}

// helper function to insert spaces before moves in a scramble
function insertSpaces(input, puzzleType = '3x3') {
	let keys;
	if (puzzleType === '3x3') {
		keys = ["U", "U2", "U2'", "U'", "D", "D2", "D2'", "D'", "L", "L2", "L2'", "L'", "R", "R2", "R2'", "R'", "F", "F2", "F2'", "F'", "B", "B2", "B2'", "B'", "u", "u2", "u2'", "u'", "d", "d2", "d2'", "d'", "l", "l2", "l2'", "l'", "r", "r2", "r2'", "r'", "f", "f2", "f2'", "f'", "b", "b2", "b2'", "b'", "Uw", "Uw2", "Uw2'", "Uw'", "Dw", "Dw2", "Dw2'", "Dw'", "Lw", "Lw2", "Lw2'", "Lw'", "Rw", "Rw2", "Rw2'", "Rw'", "Fw", "Fw2", "Fw2'", "Fw'", "Bw", "Bw2", "Bw2'", "Bw'", "M", "M2", "M2'", "M'", "S", "S2", "S2'", "S'", "E", "E2", "E2'", "E'", "x", "x2", "x2'", "x'", "y", "y2", "y2'", "y'", "z", "z2", "z2'", "z'"];
	} else if (puzzleType === '2x2') {
		keys = ["U", "U2", "U2'", "U'", "D", "D2", "D2'", "D'", "L", "L2", "L2'", "L'", "R", "R2", "R2'", "R'", "F", "F2", "F2'", "F'", "B", "B2", "B2'", "B'", "x", "x2", "x2'", "x'", "y", "y2", "y2'", "y'", "z", "z2", "z2'", "z'"];
	}
	// sort by length desc so tokens like "U2" match before "U"
	keys.sort((a, b) => b.length - a.length);
	const regex = new RegExp(`(${keys.join('|')})`, 'g');
	return input.replace(regex, ' $1').trim().replace(/\s+/g, ' ');
}

function scr_fix2(scr, puzzleType = '3x3') {
	const outputString = scr.split('\n').map(line => {
		const commentIndex = line.indexOf('//');
		if (commentIndex !== -1) {
			const beforeComment = insertSpaces(line.slice(0, commentIndex), puzzleType);
			const comment = line.slice(commentIndex);
			return beforeComment + ' ' + comment.trim();
		} else {
			return insertSpaces(line, puzzleType);
		}
	}).join('\n');
	return outputString;
}

function scr_fix(scr, puzzleType = '3x3') {
	const fixed_scr = scr_fix2(scr, puzzleType);
	const output = fixed_scr.split('\n')
		.map(line => line.split('//')[0].trim())
		.filter(line => line.length > 0)
		.join('\n');
	return output;
}

function scrambleFilter(scrStr, removeComments = true) {
	let s = scr_fix2(scrStr);
	if (removeComments) return scr_fix(scrStr);
	return s;
}

async function invertAlg(algStr, puzzleType = '3x3') {
	const Module = await _loadCppFunctionModule();
	let c_reverse = null;
	// Prefer direct exported function if present
	if (Module && typeof Module.scr_reverse === 'function') {
		c_reverse = Module.scr_reverse;
	} else if (Module && typeof Module.cwrap === 'function') {
		// try several candidate exported names with cwrap
		const candidates = ['scr_reverse', 'reverse_alg', 'reverse'];
		for (const name of candidates) {
			try {
				const fn = Module.cwrap(name, 'string', ['string']);
				if (typeof fn === 'function') { c_reverse = fn; break; }
			} catch (e) { /* ignore */ }
		}
	}
	if (!c_reverse) {
		throw new Error('reverse function not available in loaded module');
	}
	const scr_fixed = scr_fix2(algStr, puzzleType);
	if (scr_fixed === "") return;
	const lines = scr_fixed.split('\n');
	const processedLines = [];
	for (let i = lines.length - 1; i >= 0; i--) {
		const line = lines[i];
		const [alg, comment] = line.split('//');
		const alg_revresed = c_reverse(alg.trim());
		if (comment) processedLines.push(`${alg_revresed} // ${comment.trim()}`);
		else processedLines.push(alg_revresed);
	}
	const result = processedLines.join('\n');
	return result.trim() + "\n";
}

async function mirrorAlg(algStr, puzzleType = '3x3') {
	const Module = await _loadCppFunctionModule();
	let c_mirror = null;
	// Prefer direct exported function if present
	if (Module && typeof Module.scr_mirror === 'function') {
		c_mirror = Module.scr_mirror;
	} else if (Module && typeof Module.cwrap === 'function') {
		const candidates = ['scr_mirror', 'mirror_alg', 'mirror'];
		for (const name of candidates) {
			try {
				const fn = Module.cwrap(name, 'string', ['string']);
				if (typeof fn === 'function') { c_mirror = fn; break; }
			} catch (e) { /* ignore */ }
		}
	}
	if (!c_mirror) {
		throw new Error('mirror function not available in loaded module');
	}
	const scr_fixed = scr_fix2(algStr, puzzleType);
	if (scr_fixed === "") return;
	const lines = scr_fixed.split('\n');
	const processedLines = [];
	for (let i = 0; i < lines.length; i++) {
		const line = lines[i];
		const [alg, comment] = line.split('//');
		const alg_mirrored = c_mirror(alg.trim());
		if (comment) processedLines.push(`${alg_mirrored} // ${comment.trim()}`);
		else processedLines.push(alg_mirrored);
	}
	const result = processedLines.join('\n');
	return result.trim() + "\n";
}

// Exports (CommonJS). Consumers may adapt for ESM/UMD as needed.
if (typeof module !== 'undefined' && module.exports) {
	module.exports = {
		init,
		detectBaseUrl,
		_loadCppFunctionModule,
		scrambleFilter,
		scr_fix,
		scr_fix2,
		insertSpaces,
		invertAlg,
		mirrorAlg
	};
}
