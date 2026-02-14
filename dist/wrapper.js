/**
 * Rubik's Solver Library - Universal Wrapper
 * 
 * A hybrid wrapper class that can be used both as a library and as an API (CDN).
 * 
 * Usage A (API-based/via CDN):
 *   import { RubiksSolver } from 'https://or18.github.io/RubiksSolverDemo/dist/wrapper.js';
 *   const solver = new RubiksSolver(); // Uses worker.js from CDN by default
 * 
 * Usage B (Library-based/Local):
 *   import { RubiksSolver } from './solvers/wrapper.js';
 *   const solver = new RubiksSolver({ workerPath: './solvers/2x2/worker.js' });
 * 
 * @version 1.0.0
 */

export class RubiksSolver {
    /**
     * Initialize the solver
     * @param {Object} options
     * @param {string} [options.workerPath] - Path to worker.js (local or URL)
     * @param {string} [options.solver='2x2solver'] - Solver type (2x2solver, crossSolver, etc.)
     * @param {string} [options.baseUrl='https://or18.github.io/RubiksSolverDemo/dist'] - Base URL when using a CDN
     */
    constructor(options = {}) {
        this.options = options;
        
        // Solver-type alias processing
        // '2x2' → '2x2solver', 'cross' → 'crossSolver' 
        const solverAliases = {
            '2x2': '2x2solver',
            'cross': 'crossSolver',
            'eocross': 'EOCrossSolver',
            'xxcross': 'xxcrossTrainer',
            'pairing': 'pairingTrainer',
            'xcross': 'xcrossTrainer'
        };
        
        const solverInput = options.solver || '2x2';
        this.solver = solverAliases[solverInput] || solverInput;
        this.baseUrl = options.baseUrl || 'https://or18.github.io/RubiksSolverDemo/dist';
        
        // If workerPath is not specified, use the CDN (API usage)
        if (!options.workerPath) {
            this.workerPath = `${this.baseUrl}/src/${this.solver}/worker.js`;
        } else {
            this.workerPath = options.workerPath;
        }
        
        this.worker = null;
        this.messageHandlers = new Map();
        this.nextHandlerId = 0;
    }

    /**
     * Initialize Worker（async）
     * To enable cross-origin support, launch Workers via Blob.
     */
    async init() {
        try {
            const isRemote = this.workerPath.startsWith('http');

            if (isRemote) {
                // Launch a Worker from a different domain using importScripts
                // This enables Workers to run even in a CDN (API usage).
                const blobContent = `importScripts('${this.workerPath}');`;
                const blob = new Blob([blobContent], { type: 'application/javascript' });
                const blobUrl = URL.createObjectURL(blob);
                this.worker = new Worker(blobUrl);
                
                // To prevent memory leaks, release Blobs after startup.
                // (Note: Some browsers may throw errors if the Worker is released immediately after launch,
                //  so consider waiting a moment before releasing it.)
                // URL.revokeObjectURL(blobUrl); // Uncomment as needed
            } else {
                // If it's a local file, it launches normally.
                this.worker = new Worker(this.workerPath);
            }

            // Configure the message handler
            this.worker.onmessage = (e) => {
                this._handleMessage(e);
            };

            this.worker.onerror = (error) => {
                console.error('Worker error:', error);
            };

            return this;
        } catch (error) {
            console.error('Failed to initialize worker:', error);
            throw error;
        }
    }

    /**
     * Processing messages from the worker
     * @private
     */
    _handleMessage(e) {
        const { type, data } = e.data;
        
        // Call all registered handlers
        for (const handler of this.messageHandlers.values()) {
            handler({ type, data });
        }
    }

    /**
     * Register a message handler
     * @param {Function} handler - (message) => void
     * @returns {number} Handler ID (to be used for removal)
     */
    onMessage(handler) {
        const id = this.nextHandlerId++;
        this.messageHandlers.set(id, handler);
        return id;
    }

    /**
     * Unregister the message handler
     * @param {number} handlerId
     */
    offMessage(handlerId) {
        this.messageHandlers.delete(handlerId);
    }

    /**
     * Search for solutions from a scramble
     * @param {Object} params - Parameters specific to the solver being used
     * @param {string} params.scramble - Scramble string (required)
     * 
     * Common optional parameters (solver-dependent):
     * @param {number} [params.maxSolutions] - Maximum number of solutions
     * @param {number} [params.maxLength] - Maximum solution length
     * @param {string} [params.rotation] - Cube rotation
     * @param {string} [params.allowedMoves] - Allowed move set
     * 
     * Note: Exact parameters vary by solver type. See solver-specific documentation.
     */
    solve(params) {
        if (!this.worker) {
            throw new Error('Worker not initialized. Call init() first.');
        }
        
        if (!params.scramble) {
            throw new Error('scramble parameter is required');
        }
        
        this.worker.postMessage(params);
    }

    /**
     * Promise-based solve that returns only the first solution found
     * @param {Object} params - Same parameters as solve()
     * @returns {Promise<string>} First solution found
     */
    async solveOne(params) {
        return new Promise((resolve, reject) => {
            const handler = ({ type, data }) => {
                if (type === 'solution') {
                    this.offMessage(handlerId);
                    resolve(data);
                } else if (type === 'error') {
                    this.offMessage(handlerId);
                    reject(new Error(data));
                } else if (type === 'done' && !resolved) {
                    this.offMessage(handlerId);
                    reject(new Error('No solution found'));
                }
            };
            
            let resolved = false;
            const handlerId = this.onMessage((msg) => {
                if (!resolved && msg.type === 'solution') {
                    resolved = true;
                }
                handler(msg);
            });
            
            this.solve(params);
        });
    }

    /**
     * Promise-based solve that returns all solutions found
     * @param {Object} params - Same parameters as solve()
     * @returns {Promise<string[]>} Array of all solutions found
     */
    async solveAll(params) {
        return new Promise((resolve, reject) => {
            const solutions = [];
            
            const handler = ({ type, data }) => {
                if (type === 'solution') {
                    solutions.push(data);
                } else if (type === 'done') {
                    this.offMessage(handlerId);
                    resolve(solutions);
                } else if (type === 'error') {
                    this.offMessage(handlerId);
                    reject(new Error(data));
                }
            };
            
            const handlerId = this.onMessage(handler);
            this.solve(params);
        });
    }

    /**
     * Terminate the worker and release memory
     */
    terminate() {
        if (this.worker) {
            this.worker.terminate();
            this.worker = null;
        }
        this.messageHandlers.clear();
    }
}

/**
 * Manager class for handling multiple solvers
 */
export class SolverManager {
    constructor(options = {}) {
        this.baseUrl = options.baseUrl || 'https://or18.github.io/RubiksSolverDemo/dist';
        this.solvers = new Map();
    }

    /**
     * Get a solver (creates one if it doesn't exist)
     * @param {string} solverType - '2x2', 'cross', 'eocross', etc.
     * @param {Object} [options] - Options for RubiksSolver
     * @returns {Promise<RubiksSolver>}
     */
    async get(solverType, options = {}) {
        if (this.solvers.has(solverType)) {
            return this.solvers.get(solverType);
        }

        const solver = new RubiksSolver({
            solver: solverType,
            baseUrl: this.baseUrl,
            ...options
        });

        await solver.init();
        this.solvers.set(solverType, solver);
        return solver;
    }

    /**
     * Terminate all solvers
     */
    terminateAll() {
        for (const solver of this.solvers.values()) {
            solver.terminate();
        }
        this.solvers.clear();
    }
}

// Default export for legacy compatibility
export default RubiksSolver;
