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
		// Allow build tools to inject a known global base at build time (bundlers)
		try {
			if (typeof window !== 'undefined') {
				if (window.__TOOLS_BASE_URL__) return _normalizeBase(window.__TOOLS_BASE_URL__);
				if (window.__SIMULATED_BUILD_BASE__) return _normalizeBase(window.__SIMULATED_BUILD_BASE__);
			}
		} catch (e) { }
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

// Global allowed move tokens per puzzle type (used by validation helpers)
const VALID_MOVE_TOKENS = {
	'3x3': new Set([
		// face turns
		'U','U2','U-','D','D2','D-','L','L2','L-','R','R2','R-','F','F2','F-','B','B2','B-',
		// middle/slice turns
		'M','M2','M-','S','S2','S-','E','E2','E-',
		// rotations
		'x','x2','x-','y','y2','y-','z','z2','z-'
	]),
	'2x2': new Set([
		// 2x2 mainly needs rotations; include face turns conservatively
		'U','U2','U-','D','D2','D-','L','L2','L-','R','R2','R-','F','F2','F-','B','B2','B-',
		'x','x2','x-','y','y2','y-','z','z2','z-'
	])
};

async function _loadCppFunctionModule(baseUrl) {
	if (_cppFunctionModulePromise) return _cppFunctionModulePromise;

	const resolvedBase = (baseUrl !== undefined && baseUrl !== null) ? baseUrl : (_initBase || detectBaseUrl());
	const normalizedBase = _normalizeBase(resolvedBase);
	const scriptUrl = normalizedBase + 'cpp-functions/functions.js';

	// Debug: announce module load start when possible
	try {
		if (typeof self !== 'undefined' && typeof self.postMessage === 'function') {
			self.postMessage({ type: 'tools_debug', event: 'load_start', scriptUrl: scriptUrl });
		} else if (typeof console !== 'undefined' && typeof console.log === 'function') {
			console.log('tools_debug: load_start', scriptUrl);
		}
	} catch (e) { /* ignore debug failures */ }

	_cppFunctionModulePromise = new Promise((resolve, reject) => {
		// Browser
		if (typeof window !== 'undefined') {
			try {
				const s = document.createElement('script');
				s.src = scriptUrl;
				s.onload = () => {
					try { if (typeof console !== 'undefined' && console.log) console.log('tools_debug: script onload', scriptUrl); } catch (e) {}
					try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'script_onload', scriptUrl: scriptUrl }); } catch (e) {}
				
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
							Promise.resolve(maybePromise).then(m => {
								try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'factory_resolved' }); else console.log('tools_debug: factory_resolved'); } catch(e){}
								try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'resolved_module_type', moduleType: typeof m }); } catch(e){}
								resolve(m);
							}).catch(reject);
						} catch (e) {
							reject(e);
						}
						return;
					}
					// Fallback to global Module
					if (window.Module) {
						const mod = window.Module;
						if (mod.calledRun) {
							try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'module_calledRun' }); else console.log('tools_debug: module_calledRun'); } catch(e){}
							return resolve(mod);
						}
						if (typeof mod.onRuntimeInitialized === 'function') {
							const orig = mod.onRuntimeInitialized;
							mod.onRuntimeInitialized = function () {
								try { orig(); } catch (e) { /* ignore */ }
								try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'runtime_initialized' }); else console.log('tools_debug: runtime_initialized'); } catch(e){}
								resolve(mod);
							};
							return;
						}
						// fallback: poll until the module runtime is initialized or until timeout
						const start = Date.now();
						const poll = setInterval(() => {
							if (mod.calledRun || typeof mod.cwrap === 'function') {
								clearInterval(poll);
								try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'runtime_poll_resolved' }); else console.log('tools_debug: runtime_poll_resolved'); } catch(e){}
								resolve(mod);
								return;
							}
							if (Date.now() - start > 10000) { // 10s timeout
								clearInterval(poll);
								// resolve anyway; caller can handle missing symbols
								try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'runtime_poll_timeout' }); else console.log('tools_debug: runtime_poll_timeout'); } catch(e){}
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
					Promise.resolve(modFactory()).then(m => {
						try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'node_factory_resolved' }); else console.log('tools_debug: node_factory_resolved'); } catch(e){}
						resolve(m);
					}).catch(reject);
				} else {
					// modFactory is the Module object; wait until runtime is initialized
					const mod = modFactory;
					if (mod.calledRun) {
						try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'node_module_calledRun' }); else console.log('tools_debug: node_module_calledRun'); } catch(e){}
						return resolve(mod);
					}
					if (typeof mod.onRuntimeInitialized === 'function') {
						const orig = mod.onRuntimeInitialized;
						mod.onRuntimeInitialized = function () {
							try { orig(); } catch (e) { /* ignore */ }
							try { if (typeof self !== 'undefined' && typeof self.postMessage === 'function') self.postMessage({ type: 'tools_debug', event: 'node_runtime_initialized' }); else console.log('tools_debug: node_runtime_initialized'); } catch(e){}
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
			// Web Worker (non-module) - use importScripts
		} else if (typeof self !== 'undefined' && typeof importScripts === 'function') {
			try {
				// importScripts synchronously loads the script in worker
				importScripts(scriptUrl);
				// Prefer MODULARIZE=1 factory
				if (typeof createCppFunctionsModule === 'function') {
					const factory = createCppFunctionsModule;
					const opts = {
						locateFile: function (path) {
							return normalizedBase + 'cpp-functions/' + path;
						}
					};
					const maybePromise = factory(opts);
					Promise.resolve(maybePromise).then(m => resolve(m)).catch(reject);
					return;
				}
				// Fallback to global Module on worker
				if (self.Module) {
					const mod = self.Module;
					if (mod.calledRun) return resolve(mod);
					if (typeof mod.onRuntimeInitialized === 'function') {
						const orig = mod.onRuntimeInitialized;
						mod.onRuntimeInitialized = function () {
							try { orig(); } catch (e) { }
							resolve(mod);
						};
						return;
					}
					const poll = setInterval(() => {
						if (mod.calledRun || typeof mod.cwrap === 'function') {
							clearInterval(poll);
							resolve(mod);
						}
					}, 50);
					return;
				}
				return reject(new Error('Emscripten module not found in worker after importScripts ' + scriptUrl));
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

// Validate `rest` parameter (simple, returns boolean)
// - Original behavior: only letters/digits, hyphen '-' (prime), and underscore '_' allowed.
// - Apostrophe (') is NOT accepted here; caller should use '-' for prime.
// - Tokens are split by '_' and must be valid moves for the given puzzleType.
function validateRest(rest, puzzleType = '3x3') {
	if (rest === null || rest === undefined) return false;
	if (typeof rest !== 'string') return false;
	const s = rest.trim();
	if (s === '') return false; // empty is not meaningful for solver

	// Only allow alphanumerics, hyphen and underscore
	if (!/^[A-Za-z0-9_\-]+$/.test(s)) return false;

	const allowedSet = VALID_MOVE_TOKENS[puzzleType] || VALID_MOVE_TOKENS['3x3'];

	const parts = s.split('_');
	for (const p of parts) {
		if (!p || p.length === 0) return false; // empty token
		if (!allowedSet.has(p)) return false;
	}
	return true;
}

// Normalize `rest` to the form expected by the C++ input parser:
// - remove empty tokens
// - convert apostrophe "'" to hyphen '-'
// - join tokens with '_'
function normalizeRestForCpp(rest) {
	if (rest === null || rest === undefined) return '';
	if (typeof rest !== 'string') return '';
	const s = rest.trim();
	if (s === '') return '';
	const parts = s.split('_');
	const out = [];
	for (const p of parts) {
		if (!p || p.length === 0) continue;
		// convert apostrophe to hyphen for C++ expectation
		out.push(p.replace(/'/g, '-'));
	}
	return out.join('_');
}

// Build a C++-ready `rest` string from an array of move tokens using
// apostrophe notation (e.g. ["R","R2","R'"]).
// - Converts apostrophes to hyphens and joins tokens with '_'.
// - Validates the resulting string with `validateRest` and throws on error.
function buildRestFromArray(arr) {
	if (!Array.isArray(arr)) throw new Error('buildRestFromArray expects an array of strings');
	const parts = arr.map(t => (typeof t === 'string' ? t.trim() : '')).filter(Boolean);
	if (parts.length === 0) return '';
	// Join using '_' then normalize apostrophes to hyphens for C++
	const joined = parts.join('_');
	const normalized = normalizeRestForCpp(joined);
	if (!normalized) return '';
	if (!validateRest(normalized)) throw new Error('buildRestFromArray produced invalid rest: ' + normalized);
	return normalized;
}

// Build `mcv` string from an object mapping moves -> count.
// - `rest` can be an array of move tokens using apostrophe notation, or a string
//   already in (or close to) C++ form. Array input will be normalized via
//   `buildRestFromArray`.
// - `mapping` is an object like { "R": 1, "R'": 2 }
// - Returns entries joined by '_' in the form `move:count` (move uses hyphen for prime).
// - Throws on invalid input or if a referenced move is not present in `rest`.
function buildMcvFromObject(rest, mapping, puzzleType = '3x3') {
	if (mapping === null || mapping === undefined) return '';
	if (typeof mapping !== 'object' || Array.isArray(mapping)) throw new Error('buildMcvFromObject expects mapping object');

	let restStr = '';
	if (Array.isArray(rest)) {
		restStr = buildRestFromArray(rest);
	} else if (typeof rest === 'string') {
		// allow callers to pass a near-human rest string; normalize apostrophes
		restStr = normalizeRestForCpp(rest);
	} else {
		throw new Error('buildMcvFromObject expects rest as array or string');
	}

	if (!restStr) throw new Error('rest is empty after normalization');
	if (!validateRest(restStr, puzzleType)) throw new Error('rest is not valid: ' + restStr);

	const restTokens = new Set(restStr.split('_').filter(Boolean));
	const entries = [];
	for (const key of Object.keys(mapping)) {
		const rawMove = (typeof key === 'string') ? key.trim() : '';
		if (!rawMove) continue;
		const move = rawMove.replace(/'/g, '-');
		const countVal = mapping[key];
		const count = (typeof countVal === 'number') ? countVal : (typeof countVal === 'string' && /^\d+$/.test(countVal) ? Number(countVal) : NaN);
		if (!Number.isInteger(count) || count < 0) throw new Error('Invalid count for move ' + rawMove);
		if (!restTokens.has(move)) throw new Error('Move not present in rest: ' + rawMove + ' -> ' + move);
		entries.push(move + ':' + String(count));
	}
	if (entries.length === 0) return '';
	return entries.join('_');
}

// Build `mav` string from an array of [left, right] pairs.
// - `rest` can be an array of move tokens using apostrophe notation, or a string
//   already in (or close to) C++ form. Array input will be normalized via
//   `buildRestFromArray`.
// - `pairs` is an array like [["LEFT","RIGHT"], ["EMPTY","R"]]
// - Returns entries joined by '|' in the form `left~right` (left may be 'EMPTY').
// - Throws on invalid input or if a referenced move is not present in `rest`.
function buildMavFromPairs(rest, pairs, puzzleType = '3x3') {
	if (pairs === null || pairs === undefined) return '';
	if (!Array.isArray(pairs)) throw new Error('buildMavFromPairs expects an array of pairs');

	let restStr = '';
	if (Array.isArray(rest)) {
		restStr = buildRestFromArray(rest);
	} else if (typeof rest === 'string') {
		restStr = normalizeRestForCpp(rest);
	} else {
		throw new Error('buildMavFromPairs expects rest as array or string');
	}

	if (!restStr) throw new Error('rest is empty after normalization');
	if (!validateRest(restStr, puzzleType)) throw new Error('rest is not valid: ' + restStr);

	const restTokens = new Set(restStr.split('_').filter(Boolean));
	const entries = [];
	const seen = new Set();

	for (const p of pairs) {
		if (!Array.isArray(p) || p.length !== 2) throw new Error('each mav pair must be an array of two elements');
		let leftRaw = (typeof p[0] === 'string') ? p[0].trim() : '';
		let rightRaw = (typeof p[1] === 'string') ? p[1].trim() : '';

		// normalize apostrophe to hyphen
		const left = leftRaw === '' ? 'EMPTY' : leftRaw.replace(/'/g, '-');
		const right = rightRaw.replace(/'/g, '-');

		if (right === '') throw new Error('mav pair right-hand side cannot be empty');

		if (!(left === 'EMPTY' || restTokens.has(left))) throw new Error('Left token not in rest: ' + leftRaw);
		if (!restTokens.has(right)) throw new Error('Right token not in rest: ' + rightRaw);

		const repr = left + '~' + right;
		if (!seen.has(repr)) { seen.add(repr); entries.push(repr); }
	}

	if (entries.length === 0) return '';
	const out = entries.join('|');
	if (!validateMav(restStr, out, puzzleType)) throw new Error('buildMavFromPairs produced invalid mav: ' + out);
	return out;
}

// Build `crest` string from an array of [row, col] pairs.
// Each pair element is a string using apostrophe notation (e.g. "x", "x y", "EMPTY").
// - Accepts either a single pair like ["x", "x y"] or an array of pairs.
// - Converts apostrophes to hyphens, maps empty string to 'EMPTY', and joins pairs with '|'.
// - Validates result with `validateCrest` and throws on error.
async function buildCrestFromArray(pairs) {
	if (pairs === null || pairs === undefined) return '';

	let list = pairs;
	if (!Array.isArray(list)) throw new Error('buildCrestFromArray expects an array');

	const entries = [];
	const isArrayOfStrings = list.every(it => typeof it === 'string');
	if (isArrayOfStrings) {
		for (const s of list) {
			const tok = s.trim();
			if (tok === '') {
				entries.push('EMPTY_EMPTY');
				continue;
			}
			const parts = tok.split(/\s+/);
			if (parts.length === 1) {
				// Use getCenterOffset which internally calls splitRotationAlg(scr_fix(...)).rotation
				const centerStr = await getCenterOffset(tok);
				if (!centerStr || centerStr.trim() === '') {
					entries.push('EMPTY_EMPTY');
				} else {
					const normalized = centerStr.replace(/'/g, '-').replace(/\s+/g, '_');
					entries.push(normalized);
				}
			} else if (parts.length === 2) {
				let row = parts[0].replace(/'/g, '-');
				let col = parts[1].replace(/'/g, '-');
				if (row === '') row = 'EMPTY';
				if (col === '') col = 'EMPTY';
				entries.push(row + '_' + col);
			} else {
				throw new Error('invalid crest token: ' + s);
			}
		}
	} else {
		for (const item of list) {
			if (!Array.isArray(item) || item.length !== 2) throw new Error('each crest entry must be a [row, col] pair');
			let row = (typeof item[0] === 'string') ? item[0].trim() : '';
			let col = (typeof item[1] === 'string') ? item[1].trim() : '';

			if (row === '') row = 'EMPTY';
			if (col === '') col = 'EMPTY';

			// convert apostrophe to hyphen for C++/validator expectation
			row = row.replace(/'/g, '-');
			col = col.replace(/'/g, '-');

			entries.push(row + '_' + col);
		}
	}

	// Remove duplicates while preserving order
	const unique = [];
	const seen = new Set();
	for (const e of entries) {
		if (!seen.has(e)) { seen.add(e); unique.push(e); }
	}

	const out = unique.join('|');
	if (!out) return '';
	if (!validateCrest(out)) throw new Error('buildCrestFromArray produced invalid crest: ' + out);
	return out;
}

// Validate `mav` parameter: expects pairs joined by `|`, each pair is `left~right`.
// Preconditions: `rest` must be valid (checked by caller here).
function validateMav(rest, mav, puzzleType = '3x3') {
	// require rest to be valid first
	if (!validateRest(rest, puzzleType)) return false;

	if (mav === null || mav === undefined) return true; // optional
	if (typeof mav !== 'string') return false;
	const s = mav.trim();
	if (s === '') return true;

	// Only allow alphanumerics, underscore, hyphen, tilde, pipe
	if (!/^[A-Za-z0-9_\-~|]+$/.test(s)) return false;

	// No leading or trailing pipe, and no empty sections
	if (s.startsWith('|') || s.endsWith('|')) return false;

	const sections = s.split('|');
	const allowedSet = VALID_MOVE_TOKENS[puzzleType] || VALID_MOVE_TOKENS['3x3'];

	for (const sec of sections) {
		if (!sec) return false;
		const parts = sec.split('~');
		if (parts.length !== 2) return false; // must be a pair
		const left = parts[0];
		const right = parts[1];

		// left must be either 'EMPTY' or allowed token
		if (!(left === 'EMPTY' || allowedSet.has(left))) return false;
		// right must be allowed token (cannot be EMPTY on right)
		if (right === 'EMPTY') return false;
		if (!allowedSet.has(right)) return false;

		// If left is EMPTY, right must NOT be EMPTY (already enforced)
	}
	return true;
}

// Validate `mcv` parameter: expects entries like `Move:count` joined by `_`.
// Preconditions: `rest` must be valid (caller ensures). Returns boolean.
function validateMcv(rest, mcv, puzzleType = '3x3') {
	if (!validateRest(rest, puzzleType)) return false;

	if (mcv === null || mcv === undefined) return true;
	if (typeof mcv !== 'string') return false;
	const s = mcv.trim();
	if (s === '') return true;

	// Allowed characters: alnum, underscore, hyphen, colon
	if (!/^[A-Za-z0-9_\-:]+$/.test(s)) return false;

	// No leading/trailing underscore
	if (s.startsWith('_') || s.endsWith('_')) return false;

	const entries = s.split('_');
	const restTokens = new Set((rest || '').split('_').filter(Boolean));

	for (const entry of entries) {
		if (!entry) return false;
		const parts = entry.split(':');
		if (parts.length !== 2) return false;
		const move = parts[0];
		const count = parts[1];

		// count must be a non-negative integer
		if (!/^\d+$/.test(count)) return false;

		// move must appear in rest (use normalized form for comparison)
		if (!restTokens.has(move)) return false;
	}
	return true;
}

// Validate `crest` parameter.
// Accepts either a 6-char hexadecimal string (encoded) or pipe-separated pattern names.

// Allowed sanitized keys (matches crest_mapping keys in C++ buildCenterOffset)
const CREST_ALLOWED_KEYS = new Set([
	"", "y", "y2", "y'", "z2", "z2 y", "z2 y2", "z2 y'", "z'", "z' y", "z' y2", "z' y'",
	"z", "z y", "z y2", "z y'", "x'", "x' y", "x' y2", "x' y'", "x", "x y", "x y2", "x y'"
]);

function validateCrest(crest) {
	if (crest === null || crest === undefined) return true;
	if (typeof crest !== 'string') return false;
	const s = crest.trim();
	if (s === '') return true;

	// Allow 6-hex encoded form
	if (/^[0-9A-Fa-f]{6}$/.test(s)) return true;

	// Otherwise expect pipe-separated pattern names; sanitize each and validate against allowed keys
	const parts = s.split('|');
	for (const p of parts) {
		// C++ implementation skips empty id parts; accept and continue
		if (!p || p.trim() === '') continue;
		// Expect row_col format separated by '_'
		const delimPos = p.indexOf('_');
		if (delimPos === -1) return false;
		const rowRaw = p.substring(0, delimPos);
		const colRaw = p.substring(delimPos + 1);

		const sanitize = (tok) => {
			if (tok === 'EMPTY') return '';
			// replace '-' with "'" as C++ sanitize does
			return tok.replace(/-/g, "'");
		};

		const rowPart = sanitize(rowRaw);
		const colPart = sanitize(colRaw);

		let keyString = rowPart;
		if (rowPart !== '' && colPart !== '') keyString += ' ';
		keyString += colPart;

		if (!CREST_ALLOWED_KEYS.has(keyString)) return false;
	}
	return true;
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

async function splitRotationAlg(scramble, puzzleType = '3x3') {
	const Module = await _loadCppFunctionModule();
	let c_convert = null;
	if (Module && typeof Module.scr_converter === 'function') {
		c_convert = Module.scr_converter;
	} else if (Module && typeof Module.cwrap === 'function') {
		const candidates = ['scr_converter', 'convert_scramble', 'convert'];
		for (const name of candidates) {
			try {
				const fn = Module.cwrap(name, 'string', ['string']);
				if (typeof fn === 'function') { c_convert = fn; break; }
			} catch (e) { /* ignore */ }
		}
	}
	if (!c_convert) {
		throw new Error('convert function not available in loaded module');
	}
	const result = c_convert(scr_fix(scramble, puzzleType));
	const resultArray = result.split(',');
	return { rotation: resultArray[1].trim(), scramble: resultArray[0].trim() };
}

async function generateTwoPhaseInput(scramble, puzzleType = '3x3') {
	const Module = await _loadCppFunctionModule();
	let c_fn = null;
	if (Module && typeof Module.ScrambleToState === 'function') {
		c_fn = Module.ScrambleToState;
	} else if (Module && typeof Module.cwrap === 'function') {
		try {
			c_fn = Module.cwrap('ScrambleToState', 'string', ['string']);
		} catch (e) { /* ignore */ }
	}
	if (!c_fn) {
		throw new Error('ScrambleToState not available in loaded module');
	}
	const parts = await splitRotationAlg(scramble, puzzleType);
	return c_fn(scr_fix(parts.scramble, puzzleType));
}

async function rotateAlg(scramble, rotation_scramble, puzzleType = '3x3') {
	const Module = await _loadCppFunctionModule();
	let c_rot = null;
	if (Module && typeof Module.AlgRotation === 'function') {
		c_rot = Module.AlgRotation;
	} else if (Module && typeof Module.cwrap === 'function') {
		try {
			c_rot = Module.cwrap('AlgRotation', 'string', ['string', 'string']);
		} catch (e) { /* ignore */ }
	}
	if (!c_rot) {
		throw new Error('AlgRotation not available in loaded module');
	}
	return c_rot(scr_fix(scramble, puzzleType), scr_fix(rotation_scramble, puzzleType));
}

// Get center offset string for a premove token via C++ helper.
// - Accepts a premove string (accepts apostrophe; will normalize to hyphen).
// - Returns a string like 'z2 y2' that describes center offset patterns.
async function getCenterOffset(premove, puzzleType = '3x3') {
	const Module = await _loadCppFunctionModule();
	let c_fn = null;
	if (Module && typeof Module.get_center === 'function') {
		c_fn = Module.get_center;
	} else if (Module && typeof Module.cwrap === 'function') {
		const candidates = ['get_center', 'get_center_offset_param', 'get_center_offset'];
		for (const name of candidates) {
			try {
				const fn = Module.cwrap(name, 'string', ['string']);
				if (typeof fn === 'function') { c_fn = fn; break; }
			} catch (e) { /* ignore */ }
		}
	}
	if (!c_fn) {
		throw new Error('get_center function not available in loaded module');
	}
	// Preprocess: ensure we pass the rotation string produced by splitRotationAlg(scr_fix(input)).rotation
	let argStr = '';
	if (typeof premove === 'string') {
		const rotObj = await splitRotationAlg(premove, puzzleType);
		argStr = (rotObj && rotObj.rotation) ? rotObj.rotation : (premove || '');
	}
	// pass apostrophe-style rotation directly to C++ (C++ will accept it and return hyphen form)
	return c_fn(argStr);
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
		mirrorAlg,
		validateRest,
		normalizeRestForCpp,
		buildRestFromArray,
		buildMcvFromObject,
		buildMavFromPairs,
		buildCrestFromArray,
		getCenterOffset,
		validateMav,
		validateMcv,
		validateCrest,
		splitRotationAlg,
		generateTwoPhaseInput,
		rotateAlg
	};
}

// Expose a well-known global object for ESM/module consumers (including
// module workers) that import the script as a non-ESM script. Some runtime
// environments perform a dynamic `import()` but the file isn't an ES module,
// resulting in an empty module namespace; attaching a global allows such
// consumers to access the API via `self.__TOOLS_UTILS_EXPORTS__` or
// `window.__TOOLS_UTILS_EXPORTS__` as a fallback.
try {
	const exportsObj = {
		init,
		detectBaseUrl,
		_loadCppFunctionModule,
		scrambleFilter,
		scr_fix,
		scr_fix2,
		insertSpaces,
		invertAlg,
		mirrorAlg,
		validateRest,
		normalizeRestForCpp,
		buildRestFromArray,
		buildMcvFromObject,
		buildMavFromPairs,
		buildCrestFromArray,
		getCenterOffset,
		validateMav,
		validateMcv,
		validateCrest,
		splitRotationAlg,
		generateTwoPhaseInput,
		rotateAlg
	};
	if (typeof self !== 'undefined') {
		try { self.__TOOLS_UTILS_EXPORTS__ = exportsObj; } catch (e) { /* ignore */ }
	}
	if (typeof window !== 'undefined') {
		try { window.__TOOLS_UTILS_EXPORTS__ = exportsObj; } catch (e) { /* ignore */ }
	}
} catch (e) { /* ignore */ }
