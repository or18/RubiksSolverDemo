/**
 * 2x2x2 Solver Helper - Simplified Promise-based API for Web Workers
 * @version 1.2.0 (2026-02-15) - Dynamic script loading support
 * 
 * This helper wraps the Web Worker interface to provide a simple Promise-based API.
 * No need to handle worker.onmessage manually!
 * 
 * Features:
 * - CDN auto-detection (works with both <script src> and dynamic createElement)
 * - Cache busting propagation (?v=timestamp → worker.js + solver.wasm)
 * - CORS bypass via Blob URL for CDN workers
 * 
 * @example
 * // Basic usage
 * const helper = new Solver2x2Helper();
 * await helper.init();
 * 
 * const solutions = await helper.solve("R U R' U'");
 * console.log(solutions); // ['U R U\' R\'', 'R\' U\' R\' U R U', ...]
 * 
 * @example
 * // With options
 * const solutions = await helper.solve("R U R' U'", {
 *   maxSolutions: 5,
 *   maxLength: 12,
 *   onProgress: (depth) => console.log(`Searching depth ${depth}`)
 * });
 * 
 * @example
 * // Cleanup when done
 * helper.terminate();
 */
class Solver2x2Helper {
  constructor(workerPath = null) {
    // Auto-detect worker path based on how this script was loaded
    if (!workerPath) {
      let scriptUrl = null;
      
      // Method 1: Try document.currentScript (works for inline <script src="...">)
      if (typeof document !== 'undefined' && document.currentScript && document.currentScript.src) {
        scriptUrl = document.currentScript.src;
      } 
      // Method 2: Search all script tags for solver-helper.js (works for dynamic loading)
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
        // Extract base URL (without filename)
        const lastSlashIndex = scriptUrl.lastIndexOf('/');
        const baseUrl = scriptUrl.substring(0, lastSlashIndex + 1);
        
        // Extract query parameters (e.g., ?v=timestamp for cache busting)
        const queryIndex = scriptUrl.indexOf('?', lastSlashIndex);
        const queryParams = queryIndex !== -1 ? scriptUrl.substring(queryIndex) : '';
        
        // Apply same query params to worker (for cache busting)
        this.workerPath = baseUrl + 'worker_persistent.js' + queryParams;
        this.cacheBustParams = queryParams; // Store for later use
      } else {
        // Fallback to relative path
        this.workerPath = './worker_persistent.js';
        this.cacheBustParams = '';
      }
    } else {
      this.workerPath = workerPath;
      // Extract cache bust params from provided path if present
      const queryIndex = workerPath.indexOf('?');
      this.cacheBustParams = queryIndex !== -1 ? workerPath.substring(queryIndex) : '';
    }
    
    this.worker = null;
    this.ready = false;
    this.currentResolve = null;
    this.currentReject = null;
    this.currentSolutions = [];
    this.currentProgressCallback = null;
  }
  
  /**
   * Initialize the worker. Must be called before solve().
   * @returns {Promise<void>}
   */
  async init() {
    if (this.ready) {
      return; // Already initialized
    }
    
    return new Promise(async (resolve, reject) => {
      try {
        // Check if worker path is from CDN (different origin)
        const isCDN = this.workerPath.startsWith('http://') || this.workerPath.startsWith('https://');
        
        if (isCDN) {
          // Use Blob URL technique to bypass CORS
          this.worker = await this._createWorkerFromCDN(this.workerPath);
        } else {
          // Local worker
          this.worker = new Worker(this.workerPath);
        }
        
        this.worker.onmessage = (event) => {
          const msg = event.data;
          
          if (msg.type === 'ready') {
            this.ready = true;
            resolve();
          } else if (msg.type === 'solution') {
            if (msg.data && this.currentSolutions) {
              this.currentSolutions.push(msg.data);
            }
          } else if (msg.type === 'depth') {
            if (this.currentProgressCallback) {
              const depthMatch = msg.data.match(/depth=(\d+)/);
              if (depthMatch) {
                this.currentProgressCallback(parseInt(depthMatch[1]));
              }
            }
          } else if (msg.type === 'done') {
            if (this.currentResolve) {
              this.currentResolve(this.currentSolutions);
              this.currentResolve = null;
              this.currentReject = null;
              this.currentSolutions = [];
              this.currentProgressCallback = null;
            }
          } else if (msg.type === 'error') {
            if (this.currentReject) {
              this.currentReject(new Error(msg.data));
              this.currentResolve = null;
              this.currentReject = null;
              this.currentSolutions = [];
              this.currentProgressCallback = null;
            }
          }
        };
        
        this.worker.onerror = (error) => {
          if (this.currentReject) {
            this.currentReject(error);
          } else {
            reject(error);
          }
        };
        
      } catch (error) {
        reject(error);
      }
    });
  }
  
  /**
   * Solve a scramble and return all solutions.
   * 
   * @param {string} scramble - Space-separated moves (e.g., "R U R' U'")
   * @param {Object} options - Solving options
   * @param {string} [options.rotation=''] - Cube rotation (e.g., 'y', 'x2')
   * @param {number} [options.maxSolutions=3] - Max number of solutions (no upper limit)
   * @param {number} [options.maxLength=11] - Max solution length
   * @param {number} [options.pruneDepth=1] - Pruning depth (0-20, 1 recommended for URF)
   * @param {string} [options.allowedMoves='U_U2_U-_R_R2_R-_F_F2_F-'] - Allowed moves
   * @param {string} [options.preMove=''] - Moves to prepend to solutions
   * @param {string} [options.moveOrder=''] - Custom move ordering
   * @param {string} [options.moveCount=''] - Move count restrictions
   * @param {Function} [options.onProgress] - Progress callback: (depth: number) => void
   * @returns {Promise<string[]>} Array of solution strings
   */
  async solve(scramble, options = {}) {
    if (!this.ready) {
      throw new Error('Helper not initialized. Call init() first.');
    }
    
    if (this.currentResolve) {
      throw new Error('Another solve is in progress. Wait for it to complete.');
    }
    
    const {
      rotation = '',
      maxSolutions = 3,
      maxLength = 11,
      pruneDepth = 1,
      allowedMoves = 'U_U2_U-_R_R2_R-_F_F2_F-',
      preMove = '',
      moveOrder = '',
      moveCount = '',
      onProgress = null
    } = options;
    
    this.currentSolutions = [];
    this.currentProgressCallback = onProgress;
    
    return new Promise((resolve, reject) => {
      this.currentResolve = resolve;
      this.currentReject = reject;
      
      this.worker.postMessage({
        scramble,
        rotation,
        maxSolutions,
        maxLength,
        pruneDepth,
        allowedMoves,
        preMove,
        moveOrder,
        moveCount
      });
    });
  }
  
  /**
   * Terminate the worker and release resources.
   * After calling this, you cannot solve anymore unless you call init() again.
   */
  terminate() {
    if (this.worker) {
      this.worker.terminate();
      this.worker = null;
      this.ready = false;
      this.currentResolve = null;
      this.currentReject = null;
      this.currentSolutions = [];
      this.currentProgressCallback = null;
    }
  }
  
  /**
   * Reset the solver (terminate and reinitialize).
   * This will clear the pruning table and start fresh.
   * @returns {Promise<void>}
   */
  async reset() {
    this.terminate();
    await this.init();
  }
  
  /**
   * Create Worker from CDN URL using Blob URL technique to bypass CORS.
   * @private
   * @param {string} workerUrl - Full URL to worker script
   * @returns {Promise<Worker>}
   */
  async _createWorkerFromCDN(workerUrl) {
    const res = await fetch(workerUrl);
    if (!res.ok) {
      throw new Error(`Failed to fetch worker from ${workerUrl}: ${res.status} ${res.statusText}`);
    }
    
    let code = await res.text();
    
    // Extract base URL from worker URL (strip query params)
    const urlWithoutQuery = workerUrl.split('?')[0];
    const baseURL = urlWithoutQuery.substring(0, urlWithoutQuery.lastIndexOf('/') + 1);
    
    // Replace baseURL calculation in worker code
    const oldCode = `const scriptPath = self.location.href;
const baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);`;
    const newCode = `const baseURL = '${baseURL}';`;
    code = code.replace(oldCode, newCode);
    
    // Apply cache busting to solver.js and solver.wasm if present
    if (this.cacheBustParams) {
      code = code.replace(/(['"`])solver\.js\1/g, `$1solver.js${this.cacheBustParams}$1`);
      code = code.replace(/(['"`])solver\.wasm\1/g, `$1solver.wasm${this.cacheBustParams}$1`);
    }
    
    // Create blob and worker
    const blob = new Blob([code], { type: 'application/javascript' });
    return new Worker(URL.createObjectURL(blob));
  }
}

// Export for use as module or global
if (typeof module !== 'undefined' && module.exports) {
  module.exports = Solver2x2Helper;
} else {
  window.Solver2x2Helper = Solver2x2Helper;
  // Debug info for cache verification
  if (typeof console !== 'undefined') {
    console.log('[Solver2x2Helper] v1.2.0 loaded from:', 
      typeof document !== 'undefined' && document.currentScript ? document.currentScript.src : 'unknown');
  }
}
