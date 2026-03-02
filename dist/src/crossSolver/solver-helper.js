/**
 * crossSolver Web Helper - Simplified Promise-based API for Web Workers
 *
 * Wraps worker-persistent.js to provide a clean async/await interface
 * for all 8 PersistentSolver classes.
 *
 * Features:
 * - CDN auto-detection (works with both <script src> and dynamic createElement)
 * - Cache busting propagation (?v=timestamp → worker + solver.wasm)
 * - CORS bypass via Blob URL for CDN workers
 * - Streaming solution callbacks (onSolution, onProgress)
 * - Cancel support
 * - Prune table reuse (solver instances persist inside the worker)
 *
 * @example
 * const helper = new CrossSolverHelper();
 * await helper.init();
 *
 * const sols = await helper.solveCross("R U R' U'", { maxSolutions: 3 });
 * console.log(sols); // ['...', ...]
 *
 * @example
 * // Xcross with streaming callback
 * const sols = await helper.solveXcross("R U R' U'", 0, {
 *   maxSolutions: 2,
 *   maxLength: 10,
 *   onProgress: (d) => console.log('depth=' + d),
 *   onSolution: (s) => console.log('found:', s),
 * });
 *
 * @example
 * // LLSubsteps with array input
 * const sols = await helper.solveLLSubsteps(scramble, ['CO', 'EO'], {
 *   allowedMoves: ['U', "U'", 'U2', 'R', "R'", 'R2', 'F', "F'", 'F2'],
 * });
 *
 * @example
 * // Cancel a long-running solve
 * setTimeout(() => helper.cancel(), 500);
 * const sols = await helper.solveXcross(scramble, 2, { maxLength: 12 });
 */
class CrossSolverHelper {
  /**
   * @param {string|null} [workerPath] - Explicit path to worker-persistent.js.
   *   Defaults to auto-detected path relative to this script.
   */
  constructor(workerPath = null) {
    if (!workerPath) {
      let scriptUrl = null;

      // Method 1: document.currentScript (works for <script src="...">)
      if (typeof document !== 'undefined' && document.currentScript && document.currentScript.src) {
        scriptUrl = document.currentScript.src;
      }
      // Method 2: scan all script tags
      else if (typeof document !== 'undefined') {
        const scripts = document.getElementsByTagName('script');
        for (let i = 0; i < scripts.length; i++) {
          if (scripts[i].src && scripts[i].src.includes('solver-helper.js')) {
            scriptUrl = scripts[i].src;
            break;
          }
        }
      }

      if (scriptUrl) {
        const lastSlash = scriptUrl.lastIndexOf('/');
        const baseUrl = scriptUrl.substring(0, lastSlash + 1);
        const qIdx = scriptUrl.indexOf('?', lastSlash);
        const queryParams = qIdx !== -1 ? scriptUrl.substring(qIdx) : '';
        this.workerPath = baseUrl + 'worker-persistent.js' + queryParams;
        this._cacheBustParams = queryParams;
      } else {
        this.workerPath = './worker-persistent.js';
        this._cacheBustParams = '';
      }
    } else {
      this.workerPath = workerPath;
      const qIdx = workerPath.indexOf('?');
      this._cacheBustParams = qIdx !== -1 ? workerPath.substring(qIdx) : '';
    }

    this._worker = null;
    this._ready = false;

    // State for the in-flight solve
    this._resolve = null;
    this._reject = null;
    this._solutions = [];
    this._onProgress = null;
    this._onSolution = null;
    this._onCancel = null;
  }

  // -------------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------------

  /**
   * Initialize (load) the worker. Must be called once before any solve method.
   * Safe to call multiple times (no-op if already ready).
   * @returns {Promise<void>}
   */
  init() {
    if (this._ready) return Promise.resolve();

    return new Promise(async (resolve, reject) => {
      try {
        const isCDN = this.workerPath.startsWith('http://') || this.workerPath.startsWith('https://');
        if (isCDN) {
          this._worker = await this._createWorkerFromCDN(this.workerPath);
        } else {
          this._worker = new Worker(this.workerPath);
        }

        this._worker.onmessage = (event) => this._handleMessage(event.data, resolve, reject);
        this._worker.onerror = (err) => {
          if (this._reject) {
            this._reject(err);
            this._clearState();
          } else {
            reject(err);
          }
        };
      } catch (e) {
        reject(e);
      }
    });
  }

  /**
   * Cancel the currently running solve.
   * Resolves in-flight Promise with partial solutions; fires onCancel if set.
   * No-op if nothing is running.
   */
  cancel() {
    if (this._worker && this._resolve) {
      this._worker.postMessage({ type: 'cancel' });
    }
  }

  /**
   * Terminate the worker and release all resources.
   * Call init() again to reuse the helper.
   */
  terminate() {
    if (this._worker) {
      this._worker.terminate();
      this._worker = null;
      this._ready = false;
      this._clearState();
    }
  }

  /**
   * Terminate and re-initialize (clears all prune tables in the worker).
   * @returns {Promise<void>}
   */
  async reset() {
    this.terminate();
    await this.init();
  }

  /** @returns {boolean} */
  isReady() { return this._ready; }

  // -------------------------------------------------------------------------
  // Solve methods
  // -------------------------------------------------------------------------

  /**
   * Solve a cross (4 bottom-layer edges).
   * @param {string} scramble
   * @param {Object} [options]
   * @param {string}        [options.rotation='']
   * @param {number}        [options.maxSolutions=3]
   * @param {number}        [options.maxLength=8]
   * @param {string|string[]} [options.allowedMoves]  Default: all 18 moves.
   * @param {string}        [options.postAlg='']
   * @param {string|string[]} [options.centerOffset='']
   * @param {number}        [options.maxRotCount=0]
   * @param {string}        [options.moveAfterMove='']
   * @param {string}        [options.moveCount='']
   * @param {Function}      [options.onProgress]   (depth: number) => void
   * @param {Function}      [options.onSolution]   (solution: string) => void
   * @param {Function}      [options.onCancel]     (partialSols: string[]) => void
   * @returns {Promise<string[]>}
   */
  solveCross(scramble, options = {}) {
    return this._doSolve({ solverType: 'Cross', scramble }, options);
  }

  /**
   * Solve xcross (cross + one F2L pair).
   * @param {string} scramble
   * @param {number} slot - 0=BR, 1=BL, 2=FL, 3=FR
   * @param {Object} [options] - Same as solveCross; default maxLength=10.
   * @returns {Promise<string[]>}
   */
  solveXcross(scramble, slot, options = {}) {
    return this._doSolve({ solverType: 'Xcross', scramble, slot }, options, { maxLength: 10 });
  }

  /**
   * Solve xxcross (cross + two F2L pairs).
   * @param {string} scramble
   * @param {number} slot1
   * @param {number} slot2
   * @param {Object} [options] - Same as solveCross; default maxLength=12.
   * @returns {Promise<string[]>}
   */
  solveXxcross(scramble, slot1, slot2, options = {}) {
    return this._doSolve({ solverType: 'Xxcross', scramble, slot1, slot2 }, options, { maxLength: 12 });
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
    return this._doSolve({ solverType: 'Xxxcross', scramble, slot1, slot2, slot3 }, options, { maxLength: 14 });
  }

  /**
   * Solve xxxxcross (cross + all four F2L pairs).
   * @param {string} scramble
   * @param {Object} [options] - Same as solveCross; default maxLength=16.
   * @returns {Promise<string[]>}
   */
  solveXxxxcross(scramble, options = {}) {
    return this._doSolve({ solverType: 'Xxxxcross', scramble }, options, { maxLength: 16 });
  }

  /**
   * Solve LL substeps (EO, CO, CP, EP or combined).
   * @param {string} scramble
   * @param {string|string[]} ll - Tokens: 'CP', 'CO', 'EP', 'EO'.
   *   Pass a space-separated string or an array.
   *   Default (''/[]): no conditions enforced (xxxxcross endpoint equivalent).
   *   Examples: 'CO EO' or ['CO','EO'] → OLL; 'CP EP' or ['CP','EP'] → PLL.
   * @param {Object} [options] - Same as solveCross; default maxLength=16.
   * @returns {Promise<string[]>}
   */
  solveLLSubsteps(scramble, ll, options = {}) {
    return this._doSolve({ solverType: 'LLSubsteps', scramble, ll }, options, { maxLength: 16 });
  }

  /**
   * Solve full LL (CP+CO+EP+EO simultaneously).
   * @param {string} scramble
   * @param {Object} [options] - Same as solveCross; default maxLength=18.
   * @returns {Promise<string[]>}
   */
  solveLL(scramble, options = {}) {
    return this._doSolve({ solverType: 'LL', scramble }, options, { maxLength: 18 });
  }

  /**
   * Solve full LL with AUF.
   * @param {string} scramble
   * @param {Object} [options] - Same as solveCross; default maxLength=20.
   * @returns {Promise<string[]>}
   */
  solveLLAUF(scramble, options = {}) {
    return this._doSolve({ solverType: 'LLAUF', scramble }, options, { maxLength: 20 });
  }

  // -------------------------------------------------------------------------
  // Internal
  // -------------------------------------------------------------------------

  /** @private Build msg for worker and return a Promise resolved by worker responses. */
  _doSolve(baseMsg, options = {}, defaults = {}) {
    this._assertReady();
    if (this._resolve) {
      return Promise.reject(new Error('Another solve is in progress.'));
    }

    const {
      rotation = '',
      maxSolutions = 3,
      maxLength = defaults.maxLength || 8,
      allowedMoves = null,
      postAlg = '',
      centerOffset = '',
      maxRotCount = 0,
      moveAfterMove = '',
      moveCount = '',
      onProgress = null,
      onSolution = null,
      onCancel = null,
    } = options;

    this._solutions = [];
    this._onProgress = onProgress;
    this._onSolution = onSolution;
    this._onCancel = onCancel;

    return new Promise((resolve, reject) => {
      this._resolve = resolve;
      this._reject = reject;

      this._worker.postMessage({
        type: 'solve',
        ...baseMsg,
        rotation,
        maxSolutions,
        maxLength,
        allowedMoves: allowedMoves !== null ? allowedMoves : undefined,
        postAlg,
        centerOffset,
        maxRotCount,
        moveAfterMove,
        moveCount,
      });
    });
  }

  /** @private Dispatch incoming worker messages. */
  _handleMessage(msg, initResolve, initReject) {
    if (msg.type === 'ready') {
      this._ready = true;
      if (initResolve) initResolve();
      return;
    }

    if (msg.type === 'solution') {
      this._solutions.push(msg.data);
      if (this._onSolution) this._onSolution(msg.data);
      return;
    }

    if (msg.type === 'depth') {
      if (this._onProgress) {
        const m = msg.data.match(/depth=(\d+)/);
        if (m) this._onProgress(parseInt(m[1], 10));
      }
      return;
    }

    if (msg.type === 'done') {
      if (this._resolve) {
        this._resolve(this._solutions);
        this._clearState();
      }
      return;
    }

    if (msg.type === 'cancelled') {
      if (this._resolve) {
        if (this._onCancel) this._onCancel(this._solutions.slice());
        this._resolve(this._solutions);
        this._clearState();
      }
      return;
    }

    if (msg.type === 'error') {
      const err = new Error(msg.data);
      if (this._reject) {
        this._reject(err);
        this._clearState();
      } else if (initReject) {
        initReject(err);
      }
    }
  }

  /** @private */
  _assertReady() {
    if (!this._ready) throw new Error('CrossSolverHelper not initialized. Call init() first.');
  }

  /** @private */
  _clearState() {
    this._resolve = null;
    this._reject = null;
    this._solutions = [];
    this._onProgress = null;
    this._onSolution = null;
    this._onCancel = null;
  }

  /**
   * Create a Worker from a CDN URL using Blob URL technique to bypass CORS.
   * Also inlines solver.js into the worker blob to avoid importScripts issues.
   * @private
   */
  async _createWorkerFromCDN(workerUrl) {
    const res = await fetch(workerUrl);
    if (!res.ok) {
      throw new Error(`Failed to fetch worker from ${workerUrl}: ${res.status}`);
    }
    let code = await res.text();

    const urlWithoutQuery = workerUrl.split('?')[0];
    const baseURL = urlWithoutQuery.substring(0, urlWithoutQuery.lastIndexOf('/') + 1);

    const OLD_BASE = `const scriptPath = self.location.href;\nconst baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);`;
    const NEW_BASE = `const baseURL = '${baseURL}';`;

    if (code.includes('importScripts(')) {
      try {
        const solverUrl = baseURL + 'solver.js' + (this._cacheBustParams || '');
        const sres = await fetch(solverUrl);
        if (sres.ok) {
          const solverText = await sres.text();
          code = code.replace(/importScripts\([^)]*\);?\n?/g, '');
          code = solverText + '\n' + code;
          code = code.replace(OLD_BASE, NEW_BASE);
        } else {
          code = code.replace(OLD_BASE, NEW_BASE);
        }
      } catch (e) {
        code = code.replace(OLD_BASE, NEW_BASE);
      }
    } else {
      code = code.replace(OLD_BASE, NEW_BASE);
    }

    // Propagate cache-bust params to solver.wasm reference
    if (this._cacheBustParams) {
      code = code.replace(/(['"`])solver\.js\1/g, `$1solver.js${this._cacheBustParams}$1`);
      code = code.replace(/(['"`])solver\.wasm\1/g, `$1solver.wasm${this._cacheBustParams}$1`);
    }

    const blob = new Blob([code], { type: 'application/javascript' });
    return new Worker(URL.createObjectURL(blob));
  }
}

// Export
if (typeof module !== 'undefined' && module.exports) {
  module.exports = CrossSolverHelper;
} else if (typeof window !== 'undefined') {
  window.CrossSolverHelper = CrossSolverHelper;
}
