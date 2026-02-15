/**
 * 2x2x2 Solver Helper - Simplified Promise-based API for Web Workers
 * 
 * This helper wraps the Web Worker interface to provide a simple Promise-based API.
 * No need to handle worker.onmessage manually!
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
  constructor(workerPath = './worker_persistent.js') {
    this.workerPath = workerPath;
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
    
    return new Promise((resolve, reject) => {
      try {
        this.worker = new Worker(this.workerPath);
        
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
}

// Export for use as module or global
if (typeof module !== 'undefined' && module.exports) {
  module.exports = Solver2x2Helper;
} else {
  window.Solver2x2Helper = Solver2x2Helper;
}
