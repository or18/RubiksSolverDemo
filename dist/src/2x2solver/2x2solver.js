/**
 * 2x2 Solver - Simple JavaScript API
 * 
 * A user-friendly wrapper around the 2x2 solver WebAssembly module.
 * 
 * @example
 * ```javascript
 * import { Solver2x2 } from './2x2solver.js';
 * 
 * const solver = new Solver2x2();
 * 
 * // Simple usage
 * const solutions = await solver.solve("R U R' U'");
 * console.log(solutions); // ['U R U\' R\'', 'R\' U\' R\' U R U', ...]
 * 
 * // With options
 * const solutions = await solver.solve("R U R' U'", {
 *   maxSolutions: 10,
 *   maxLength: 15,
 *   rotation: "y"
 * });
 * ```
 */

export class Solver2x2 {
  constructor() {
    this.worker = null;
    this.workerReady = false;
  }

  /**
   * Solve a 2x2 cube scramble
   * 
   * @param {string} scramble - The scramble to solve
   * @param {Object} options - Optional solving parameters
   * @param {string} [options.rotation=""] - Whole cube rotation (e.g., "y", "z")
   * @param {number} [options.maxSolutions=20] - Maximum number of solutions
   * @param {number} [options.maxLength=20] - Maximum solution length
   * @param {number} [options.pruneDepth=8] - Pruning depth (1-11)
   * @param {string} [options.allowedMoves="U_U2_U-_R_R2_R-_F_F2_F-"] - Allowed moves
   * @param {string} [options.preMove=""] - Pre-move sequence
   * @param {string} [options.moveOrder=""] - Move order constraints
   * @param {string} [options.moveCount=""] - Move count limits
   * @returns {Promise<string[]>} Array of solution strings
   */
  async solve(scramble, options = {}) {
    if (!scramble || typeof scramble !== 'string') {
      throw new Error('Scramble must be a non-empty string');
    }

    const solutions = [];
    
    return new Promise((resolve, reject) => {
      // Create worker
      this.worker = new Worker(new URL('./worker.js', import.meta.url));
      
      // Handle messages
      this.worker.onmessage = (e) => {
        const { type, data } = e.data;
        
        if (type === 'solution') {
          solutions.push(data);
        } else if (type === 'done') {
          this.worker.terminate();
          this.worker = null;
          resolve(solutions);
        } else if (type === 'error') {
          this.worker.terminate();
          this.worker = null;
          reject(new Error(data));
        }
      };
      
      this.worker.onerror = (error) => {
        this.worker.terminate();
        this.worker = null;
        reject(error);
      };
      
      // Send parameters with defaults
      const params = {
        scramble,
        rotation: options.rotation !== undefined ? options.rotation : "",
        maxSolutions: options.maxSolutions !== undefined ? options.maxSolutions : 20,
        maxLength: options.maxLength !== undefined ? options.maxLength : 20,
        pruneDepth: options.pruneDepth !== undefined ? options.pruneDepth : 8,
        allowedMoves: options.allowedMoves !== undefined ? options.allowedMoves : "U_U2_U-_R_R2_R-_F_F2_F-",
        preMove: options.preMove !== undefined ? options.preMove : "",
        moveOrder: options.moveOrder !== undefined ? options.moveOrder : "",
        moveCount: options.moveCount !== undefined ? options.moveCount : ""
      };
      
      this.worker.postMessage(params);
    });
  }

  /**
   * Solve with event callbacks (for streaming results)
   * 
   * @param {string} scramble - The scramble to solve
   * @param {Object} options - Optional solving parameters
   * @param {Function} options.onSolution - Called for each solution found
   * @param {Function} options.onDone - Called when search completes
   * @param {Function} options.onError - Called if an error occurs
   * @returns {void}
   */
  search(scramble, options = {}) {
    if (!scramble || typeof scramble !== 'string') {
      throw new Error('Scramble must be a non-empty string');
    }

    // Create worker
    this.worker = new Worker(new URL('./worker.js', import.meta.url));
    
    // Handle messages
    this.worker.onmessage = (e) => {
      const { type, data } = e.data;
      
      if (type === 'solution' && options.onSolution) {
        options.onSolution(data);
      } else if (type === 'done') {
        if (options.onDone) options.onDone();
        this.worker.terminate();
        this.worker = null;
      } else if (type === 'error') {
        if (options.onError) options.onError(new Error(data));
        this.worker.terminate();
        this.worker = null;
      }
    };
    
    this.worker.onerror = (error) => {
      if (options.onError) options.onError(error);
      this.worker.terminate();
      this.worker = null;
    };
    
    // Send parameters with defaults
    const params = {
      scramble,
      rotation: options.rotation !== undefined ? options.rotation : "",
      maxSolutions: options.maxSolutions !== undefined ? options.maxSolutions : 20,
      maxLength: options.maxLength !== undefined ? options.maxLength : 20,
      pruneDepth: options.pruneDepth !== undefined ? options.pruneDepth : 8,
      allowedMoves: options.allowedMoves !== undefined ? options.allowedMoves : "U_U2_U-_R_R2_R-_F_F2_F-",
      preMove: options.preMove !== undefined ? options.preMove : "",
      moveOrder: options.moveOrder !== undefined ? options.moveOrder : "",
      moveCount: options.moveCount !== undefined ? options.moveCount : ""
    };
    
    this.worker.postMessage(params);
  }

  /**
   * Cancel the current search
   */
  cancel() {
    if (this.worker) {
      this.worker.terminate();
      this.worker = null;
    }
  }
}

/**
 * Quick solve function (convenience wrapper)
 * 
 * @param {string} scramble - The scramble to solve
 * @param {Object} options - Optional solving parameters
 * @returns {Promise<string[]>} Array of solution strings
 */
export async function solve(scramble, options = {}) {
  const solver = new Solver2x2();
  return solver.solve(scramble, options);
}
