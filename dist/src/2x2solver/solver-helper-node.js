/**
 * 2x2x2 Solver Helper for Node.js - Simplified Promise-based API
 * 
 * This helper wraps the WASM module to provide a simple Promise-based API.
 * No need to handle globalThis.postMessage manually!
 * 
 * @example
 * // ES6 modules
 * import Solver2x2HelperNode from './solver-helper-node.js';
 * 
 * const helper = new Solver2x2HelperNode();
 * await helper.init();
 * 
 * const solutions = await helper.solve("R U R' U'");
 * console.log(solutions); // ['U R U\' R\'', 'R\' U\' R\' U R U', ...]
 * 
 * @example
 * // CommonJS
 * const Solver2x2HelperNode = require('./solver-helper-node.js');
 * 
 * (async () => {
 *   const helper = new Solver2x2HelperNode();
 *   await helper.init();
 *   
 *   const solutions = await helper.solve("R U R' U'", { maxSolutions: 5 });
 *   console.log(solutions);
 * })();
 * 
 * @example
 * // With progress callback
 * const solutions = await helper.solve("R U R' U'", {
 *   onProgress: (depth) => console.log(`Searching depth ${depth}`)
 * });
 */
class Solver2x2HelperNode {
  constructor(solverPath = './solver.js') {
    this.solverPath = solverPath;
    this.Module = null;
    this.solver = null;
    this.ready = false;
  }
  
  /**
   * Initialize the WASM module. Must be called before solve().
   * @returns {Promise<void>}
   */
  async init() {
    if (this.ready) {
      return; // Already initialized
    }
    
    // Dynamically import the module (works for both CommonJS and ES6)
    let createModule;
    if (typeof require !== 'undefined') {
      // CommonJS
      createModule = require(this.solverPath);
    } else {
      // ES6 modules
      const module = await import(this.solverPath);
      createModule = module.default;
    }
    
    this.Module = await createModule();
    this.solver = new this.Module.PersistentSolver2x2();
    this.ready = true;
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
   * @param {Function} [options.onSolution] - Solution callback: (solution: string) => void
   * @returns {Promise<string[]>} Array of solution strings
   */
  async solve(scramble, options = {}) {
    if (!this.ready) {
      throw new Error('Helper not initialized. Call init() first.');
    }
    
    const {
      rotation = '',
      maxSolutions = 3,
      maxLength = 11,
      pruneDepth = 1,
      // keep legacy string inputs
      allowedMoves = 'U_U2_U-_R_R2_R-_F_F2_F-',
      preMove = '',
      moveOrder = '',
      moveCount = '',
      onProgress = null,
      onSolution = null,
      // new structured options (optional)
      // `allowedMovesArray`: allowed moves as an array (or string)
      // `moveOrderArray`: move order pairs array
      // `moveCountMap`: move count mapping object
      allowedMovesArray = null,
      moveOrderArray = null,
      moveCountMap = null
    } = options;
    
    const solutions = [];

    // Override globalThis.postMessage to capture output
    const originalPostMessage = globalThis.postMessage;

    return new Promise((resolve, reject) => {
      globalThis.postMessage = (msg) => {
        if (typeof msg === 'string') {
          if (msg === 'Search finished.') {
            // Restore original postMessage
            globalThis.postMessage = originalPostMessage;
            resolve(solutions);
          } else if (msg.startsWith('Error')) {
            // Restore original postMessage
            globalThis.postMessage = originalPostMessage;
            reject(new Error(msg));
          } else if (msg === 'Already solved.') {
            // Empty scramble - already solved
            globalThis.postMessage = originalPostMessage;
            resolve(['']); // Empty solution
          } else if (msg.startsWith('depth=')) {
            // Progress update
            if (onProgress) {
              const depthMatch = msg.match(/depth=(\d+)/);
              if (depthMatch) {
                onProgress(parseInt(depthMatch[1]));
              }
            }
          } else if (msg !== '') {
            // Normal solution string
            solutions.push(msg);
            if (onSolution) {
              onSolution(msg);
            }
          }
        }
      };

      try {
        // Use shared tools.js utilities for rest/mav/mcv conversions
        const tools = require('../utils/tools.js');

        // prefer structured `allowedMovesArray` over legacy allowedMoves string
        let restStr = '';
        if (allowedMovesArray !== null && allowedMovesArray !== undefined) {
          if (Array.isArray(allowedMovesArray)) restStr = tools.buildRestFromArray(allowedMovesArray);
          else restStr = tools.normalizeRestForCpp(allowedMovesArray);
        } else {
          restStr = (typeof allowedMoves === 'string') ? allowedMoves : '';
        }

        // moveOrder: explicit string preferred, else build from mav
        let moveOrderStr = '';
        if (moveOrder && typeof moveOrder === 'string' && moveOrder.trim() !== '') {
          moveOrderStr = moveOrder;
        } else if (moveOrderArray) {
          moveOrderStr = tools.buildMavFromPairs(restStr, moveOrderArray, '2x2');
        }

        // moveCount: explicit string preferred, else build from mcv
        let moveCountStr = '';
        if (moveCount && typeof moveCount === 'string' && moveCount.trim() !== '') {
          moveCountStr = moveCount;
        } else if (moveCountMap) {
          moveCountStr = tools.buildMcvFromObject(restStr, moveCountMap, '2x2');
        }

        const allowedMovesToPass = restStr && restStr !== '' ? restStr : allowedMoves;

        // Call solver with converted parameters
        this.solver.solve(
          scramble,
          rotation,
          maxSolutions,
          maxLength,
          pruneDepth,
          allowedMovesToPass,
          preMove,
          moveOrderStr,
          moveCountStr
        );
      } catch (error) {
        // Restore original postMessage
        globalThis.postMessage = originalPostMessage;
        reject(error);
      }
    });
  }
  
  /**
   * Reset the solver instance.
   * This will clear the pruning table and create a new solver.
   * 
   * ⚠️ WARNING: In Node.js, creating multiple solver instances can cause
   * memory issues (OOM) because WASM memory is not immediately released.
   * For most use cases, keep using the same solver instance - it will
   * automatically reuse the pruning table for better performance.
   * 
   * Only use reset() if you absolutely need a fresh solver state.
   */
  reset() {
    if (this.Module && this.solver) {
      // Note: This may cause memory issues if called frequently
      // The old solver's memory won't be released immediately
      this.solver = new this.Module.PersistentSolver2x2();
    }
  }
  
  /**
   * Check if the helper is ready to solve.
   * @returns {boolean}
   */
  isReady() {
    return this.ready;
  }
}

// Export for both CommonJS and ES6
if (typeof module !== 'undefined' && module.exports) {
  module.exports = Solver2x2HelperNode;
}

// Note: For ES6 modules, use: import Solver2x2HelperNode from './solver-helper-node.js';
// The default export is handled by the module.exports above when using CommonJS

