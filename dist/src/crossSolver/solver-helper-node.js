/**
 * crossSolver Node.js Helper - Simplified Promise-based API
 *
 * Wraps all PersistentSolver classes (Cross, Xcross, Xxcross, Xxxcross,
 * Xxxxcross, LLSubsteps, LL, LLAUF) with a clean async/await interface.
 * Solver instances are lazy-initialized and cached per slot combination
 * so prune tables are reused across multiple solve() calls.
 *
 * @example
 * // CommonJS
 * const CrossSolverHelperNode = require('./solver-helper-node.js');
 *
 * const helper = new CrossSolverHelperNode();
 * await helper.init();
 *
 * const sols = await helper.solveCross('R U R2 F D2 L B U2 F', { maxSolutions: 3 });
 * console.log(sols); // ['U F2 D B2 L\'  D ', ...]
 *
 * @example
 * // Xcross (slot 0 = back-right)
 * const sols = await helper.solveXcross('R U R2 F D D2 L B', 0, { maxSolutions: 2 });
 *
 * @example
 * // With progress + streaming callbacks
 * const sols = await helper.solveLL('R U R\' U\' R\' F R2 U\' R\' U\' R U R\' F\'', {
 *   maxSolutions: 5,
 *   allowedMoves: ['U', "U'", 'U2', 'R', "R'", 'R2', 'F', "F'", 'F2'],
 *   onProgress: (d) => process.stdout.write('depth=' + d + ' '),
 *   onSolution: (s) => console.log('Found: ' + s),
 * });
 */
class CrossSolverHelperNode {
  /**
   * @param {string} [solverPath] - Path to solver.js. Defaults to ./solver.js
   *   relative to this file's directory.
   */
  constructor(solverPath = null) {
    this.solverPath = solverPath || require('path').join(__dirname, 'solver.js');
    this.Module = null;
    this.ready = false;
    // Cached solver instances, keyed by "Type:slot1:slot2..."
    this._solvers = {};
  }

  // -------------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------------

  /**
   * Initialize the WASM module. Must be called once before any solve method.
   * @returns {Promise<void>}
   */
  async init() {
    if (this.ready) return;
    // Register globalThis.postMessage before module load so EM_JS update()
    // picks it up from the start.
    if (!globalThis.postMessage) {
      globalThis.postMessage = () => {};
    }
    let createModule;
    if (typeof require !== 'undefined') {
      createModule = require(this.solverPath);
    } else {
      const m = await import(this.solverPath);
      createModule = m.default;
    }
    this.Module = await createModule();
    this.ready = true;
  }

  /**
   * Cancel the currently running solve.
   * The in-flight solve() Promise resolves early with whatever solutions
   * were found so far; the onCancel callback (if any) is invoked.
   * No-op if nothing is running.
   */
  cancel() {
    if (this.Module) {
      this.Module._cancelRequested = true;
    }
  }

  /**
   * Set the cancel-check frequency for depth_limited_search.
   * The check fires every (mask + 1) nodes at depth >= 9.
   * Default: 0x7FFF (32768 nodes). Use 0x7FFFFFFF to disable (benchmark only).
   * @param {number} mask - Bitmask value.
   */
  setCancelCheckMask(mask) {
    if (this.Module) {
      this.Module.setCancelCheckMask(mask);
    }
  }

  /**
   * Release all cached solver instances.
   * Prune tables are freed; next solve calls will rebuild them.
   */
  reset() {
    this._solvers = {};
  }

  /** @returns {boolean} true if init() has completed */
  isReady() { return this.ready; }

  // -------------------------------------------------------------------------
  // Solve methods
  // -------------------------------------------------------------------------

  /**
   * Solve a cross (4 bottom-layer edges).
   * @param {string} scramble - Scramble moves.
   * @param {Object} [options]
   * @param {string}   [options.rotation='']         - Cube rotation (e.g. 'y x2')
   * @param {number}   [options.maxSolutions=3]      - Max solutions to return.
   * @param {number}   [options.maxLength=8]         - Max solution length (moves).
   * @param {string|string[]} [options.allowedMoves] - Allowed moves. Default: all 18.
   * @param {string}   [options.postAlg='']          - Post-solve moves.
   * @param {string|string[]} [options.centerOffset=''] - Target face orientation(s).
   *   Pass '' (default) for white-bottom standard cross.
   *   Pass an array like ['', 'y', 'y2', "y'"] to allow multiple orientations.
   *   Raw pipe-separated format (e.g. 'EMPTY_EMPTY|EMPTY_y') is also accepted.
   * @param {number}   [options.maxRotCount=0]       - Max rotation moves in solution.
   * @param {string}   [options.moveAfterMove='']    - Move-after-move restriction.
   * @param {string}   [options.moveCount='']        - Move count restriction.
   * @param {Function} [options.onProgress]          - Callback: (depth: number) => void
   * @param {Function} [options.onSolution]          - Callback: (solution: string) => void
   * @param {Function} [options.onCancel]            - Callback: (partialSols: string[]) => void
   * @returns {Promise<string[]>}
   */
  solveCross(scramble, options = {}) {
    this._assertReady();
    const s = this._getSolver('Cross');
    const a = this._args(scramble, { maxLength: 8, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  /**
   * Solve xcross (cross + one F2L pair).
   * @param {string} scramble
   * @param {number} slot - F2L slot to include: 0=BR, 1=BL, 2=FL, 3=FR
   * @param {Object} [options] - Same as solveCross; default maxLength=10.
   * @returns {Promise<string[]>}
   */
  solveXcross(scramble, slot, options = {}) {
    this._assertReady();
    const s = this._getSolver('Xcross', slot);
    const a = this._args(scramble, { maxLength: 10, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  /**
   * Solve xxcross (cross + two F2L pairs).
   * @param {string} scramble
   * @param {number} slot1 - First F2L slot (0-3).
   * @param {number} slot2 - Second F2L slot (0-3).
   * @param {Object} [options] - Same as solveCross; default maxLength=12.
   * @returns {Promise<string[]>}
   */
  solveXxcross(scramble, slot1, slot2, options = {}) {
    this._assertReady();
    const s = this._getSolver('Xxcross', slot1, slot2);
    const a = this._args(scramble, { maxLength: 12, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  /**
   * Solve xxxcross (cross + three F2L pairs).
   * @param {string} scramble
   * @param {number} slot1
   * @param {number} slot2
   * @param {number} slot3
   * @param {Object} [options] - Same as solveCross; default maxLength=14.
   * @returns {Promise<string[]>}
   */
  solveXxxcross(scramble, slot1, slot2, slot3, options = {}) {
    this._assertReady();
    const s = this._getSolver('Xxxcross', slot1, slot2, slot3);
    const a = this._args(scramble, { maxLength: 14, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  /**
   * Solve xxxxcross (cross + all four F2L pairs).
   * @param {string} scramble
   * @param {Object} [options] - Same as solveCross; default maxLength=16.
   * @returns {Promise<string[]>}
   */
  solveXxxxcross(scramble, options = {}) {
    this._assertReady();
    const s = this._getSolver('Xxxxcross');
    const a = this._args(scramble, { maxLength: 16, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  /**
   * Solve LL substeps (EO, CO, CP, EP or combined).
   * @param {string} scramble
   * @param {string|string[]} ll - Which LL conditions to enforce.
   *   Accepted tokens: 'CP', 'CO', 'EP', 'EO'.
   *   Pass a space-separated string or an array of tokens.
   *   Default (empty string / empty array): no conditions enforced (any LL state accepted;
   *   equivalent to xxxxcross endpoint).
   *   Examples:
   *     'EO'              → orient LL edges only
   *     'CO'              → orient LL corners only
   *     'CO EO'           → orient all LL pieces (OLL)
   *     'CP EP'           → permute all LL pieces (PLL)
   *     ['CP', 'CO', 'EP', 'EO'] → full LL solve
   * @param {Object} [options] - Same as solveCross; default maxLength=16.
   * @returns {Promise<string[]>}
   */
  solveLLSubsteps(scramble, ll, options = {}) {
    this._assertReady();
    const s = this._getSolver('LLSubsteps');
    const a = this._argsLL(scramble, ll, { maxLength: 16, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  /**
   * Solve full LL (all four conditions simultaneously).
   * @param {string} scramble
   * @param {Object} [options] - Same as solveCross; default maxLength=18.
   * @returns {Promise<string[]>}
   */
  solveLL(scramble, options = {}) {
    this._assertReady();
    const s = this._getSolver('LL');
    const a = this._args(scramble, { maxLength: 18, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  /**
   * Solve full LL with AUF (U-face alignment before and after).
   * @param {string} scramble
   * @param {Object} [options] - Same as solveCross; default maxLength=20.
   * @returns {Promise<string[]>}
   */
  solveLLAUF(scramble, options = {}) {
    this._assertReady();
    const s = this._getSolver('LLAUF');
    const a = this._args(scramble, { maxLength: 20, ...options });
    return this._doSolve(() => s.solve(...a), options);
  }

  // -------------------------------------------------------------------------
  // Internal helpers
  // -------------------------------------------------------------------------

  _assertReady() {
    if (!this.ready) throw new Error('CrossSolverHelperNode not initialized. Call init() first.');
  }

  _getSolver(type, ...slots) {
    const key = [type, ...slots].join(':');
    if (!this._solvers[key]) {
      const M = this.Module;
      switch (type) {
        case 'Cross':      this._solvers[key] = new M.PersistentCrossSolver(); break;
        case 'Xcross':     this._solvers[key] = new M.PersistentXcrossSolver(slots[0]); break;
        case 'Xxcross':    this._solvers[key] = new M.PersistentXxcrossSolver(slots[0], slots[1]); break;
        case 'Xxxcross':   this._solvers[key] = new M.PersistentXxxcrossSolver(slots[0], slots[1], slots[2]); break;
        case 'Xxxxcross':  this._solvers[key] = new M.PersistentXxxxcrossSolver(); break;
        case 'LLSubsteps': this._solvers[key] = new M.PersistentLLSubstepsSolver(); break;
        case 'LL':         this._solvers[key] = new M.PersistentLLSolver(); break;
        case 'LLAUF':      this._solvers[key] = new M.PersistentLLAUFSolver(); break;
        default: throw new Error('Unknown solver type: ' + type);
      }
    }
    return this._solvers[key];
  }

  /**
   * Normalize the ll parameter for LL_substeps_option_array().
   * Accepts a space-separated string like 'CP EO' or an array like ['CP', 'EO'].
   * Returns a space-separated string of known tokens (CP / CO / EP / EO).
   */
  _llStr(ll) {
    if (!ll) return '';
    if (Array.isArray(ll)) return ll.join(' ');
    return String(ll);
  }

  /** Build the moves string accepted by C++ buidMoveRestrict(). */
  _restStr(allowedMoves) {
    if (!allowedMoves) return 'U_U2_U-_D_D2_D-_R_R2_R-_L_L2_L-_F_F2_F-_B_B2_B-';
    if (Array.isArray(allowedMoves)) {
      // Convert "U'" → "U-", then join with underscore
      return allowedMoves.map(m => m.replace(/'/g, '-')).join('_');
    }
    return String(allowedMoves);
  }

  /**
   * Build the center_offset_string accepted by C++ buildCenterOffset().
   * Format: pipe-separated "row_col" pairs, where each part uses '-' for "'".
   * Empty rotation → "EMPTY_EMPTY".
   */
  _centerOffsetStr(centerOffset) {
    if (centerOffset === null || centerOffset === undefined || centerOffset === '') {
      return 'EMPTY_EMPTY';
    }
    if (Array.isArray(centerOffset)) {
      return centerOffset.map(r => {
        if (!r || r === '') return 'EMPTY_EMPTY';
        const parts = r.trim().split(/\s+/);
        const row = (parts[0] || 'EMPTY').replace(/'/g, '-');
        const col = (parts[1] || 'EMPTY').replace(/'/g, '-');
        return `${row}_${col}`;
      }).join('|');
    }
    // Raw string passed directly (e.g. 'EMPTY_EMPTY' or 'EMPTY_y|EMPTY_y2')
    return String(centerOffset);
  }

  /**
   * Build the positional args array for standard solve() calls
   * (Cross / Xcross / Xxcross / Xxxcross / Xxxxcross / LL / LLAUF).
   */
  _args(scramble, options) {
    const {
      rotation = '',
      maxSolutions = 3,
      maxLength = 8,
      allowedMoves = null,
      postAlg = '',
      centerOffset = '',
      maxRotCount = 0,
      moveAfterMove = '',
      moveCount = '',
    } = options;
    return [
      scramble, rotation, maxSolutions, maxLength,
      this._restStr(allowedMoves),
      postAlg,
      this._centerOffsetStr(centerOffset),
      maxRotCount, moveAfterMove, moveCount,
    ];
  }

  /**
   * Build args for LLSubsteps solve() (extra `ll` parameter after `rot`).
   */
  _argsLL(scramble, ll, options) {
    const {
      rotation = '',
      maxSolutions = 3,
      maxLength = 16,
      allowedMoves = null,
      postAlg = '',
      centerOffset = '',
      maxRotCount = 0,
      moveAfterMove = '',
      moveCount = '',
    } = options;
    return [
      scramble, rotation, this._llStr(ll), maxSolutions, maxLength,
      this._restStr(allowedMoves),
      postAlg,
      this._centerOffsetStr(centerOffset),
      maxRotCount, moveAfterMove, moveCount,
    ];
  }

  /**
   * Intercept globalThis.postMessage, call the solver, and return a Promise
   * that resolves with collected solutions on "Search finished." / cancel.
   */
  _doSolve(callFn, options = {}) {
    const { onProgress = null, onSolution = null, onCancel = null } = options;
    const solutions = [];
    const origPostMessage = globalThis.postMessage;

    return new Promise((resolve, reject) => {
      globalThis.postMessage = (msg) => {
        if (msg === 'Search finished.' || msg === 'Already solved.') {
          globalThis.postMessage = origPostMessage;
          resolve(solutions);
        } else if (msg === 'Search cancelled.') {
          globalThis.postMessage = origPostMessage;
          if (onCancel) onCancel(solutions.slice());
          resolve(solutions);
        } else if (msg.startsWith('depth=')) {
          if (onProgress) {
            const m = msg.match(/depth=(\d+)/);
            if (m) onProgress(parseInt(m[1], 10));
          }
        } else if (msg.startsWith('Error')) {
          globalThis.postMessage = origPostMessage;
          reject(new Error(msg));
        } else if (msg !== '') {
          solutions.push(msg);
          if (onSolution) onSolution(msg);
        }
      };
      try {
        callFn();
      } catch (e) {
        globalThis.postMessage = origPostMessage;
        reject(e);
      }
    });
  }
}

// Support both CommonJS and ES6 imports
if (typeof module !== 'undefined' && module.exports) {
  module.exports = CrossSolverHelperNode;
}
