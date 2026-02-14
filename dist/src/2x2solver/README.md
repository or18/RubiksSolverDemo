# 2x2 Cube Solver

WebAssembly-based 2x2x2 Rubik's Cube solver with a simple JavaScript API.

## Quick Start

### Option 1: Simple API (Recommended)

```javascript
import { solve } from './2x2solver.js';

// Basic usage
const solutions = await solve("R U R' U'");
console.log(solutions);
// ['U R U\' R\'', 'R\' U\' R\' U R U', ...]

// With options
const solutions = await solve("R U R' U'", {
  maxSolutions: 10,
  maxLength: 15
});
```

### Option 2: Event-based API (Streaming)

```javascript
import { Solver2x2 } from './2x2solver.js';

const solver = new Solver2x2();

solver.search("R U R' U'", {
  onSolution: (sol) => console.log('Found:', sol),
  onDone: () => console.log('Search complete'),
  onError: (err) => console.error(err)
});

// Cancel if needed
solver.cancel();
```

### Option 3: Worker API (Advanced)

Direct Web Worker usage for fine-grained control.

```javascript
const worker = new Worker('worker.js');

worker.onmessage = (e) => {
  const { type, data } = e.data;
  if (type === 'solution') {
    console.log('Solution:', data);
  } else if (type === 'done') {
    console.log('Search complete');
  } else if (type === 'error') {
    console.error('Error:', data);
  }
};

// Send solve request
worker.postMessage({
  scramble: "R U R' U'",
  maxSolutions: 20,
  maxLength: 20,
  pruneDepth: 8,
  rotation: "",
  allowedMoves: "U_U2_U-_R_R2_R-_F_F2_F-",
  preMove: "",
  moveOrder: "",
  moveCount: ""
});

// Later: terminate when done
worker.terminate();
```

**Message types received:**
- `{ type: 'solution', data: <solution_string> }` - A solution was found
- `{ type: 'done' }` - Search completed
- `{ type: 'error', data: <error_message> }` - An error occurred

**Note:** The simple API (Option 1) and class-based API (Option 2) are recommended for most use cases.

## API Reference

### `solve(scramble, options)`

Solves a scramble and returns all solutions as a Promise.

**Parameters:**
- `scramble` (string, required): The scramble to solve
- `options` (object, optional):
  - `maxSolutions` (number, default: 20): Maximum number of solutions to find
  - `maxLength` (number, default: 20): Maximum solution length in moves
  - `pruneDepth` (number, default: 8): Search depth (1-11)
  - `rotation` (string, default: ""): Whole-cube rotation
  - `allowedMoves` (string, default: "U_U2_U-_R_R2_R-_F_F2_F-"): Allowed move set
  - `preMove` (string, default: ""): Pre-move sequence
  - `moveOrder` (string, default: ""): Move order constraints
  - `moveCount` (string, default: ""): Move count limits

**Returns:** `Promise<string[]>` - Array of solution strings

**Example:**
```javascript
const solutions = await solve("R U R' U'", {
  maxSolutions: 5,
  maxLength: 12,
  pruneDepth: 9
});
```

---

## Parameter Reference

### `scramble` (string, required)

The scramble sequence to solve.

**Format:** Space-separated move notation
- Clockwise turns: `U`, `D`, `L`, `R`, `F`, `B`
- 180° turns: `U2`, `D2`, `L2`, `R2`, `F2`, `B2`
- Counter-clockwise (prime): `U'`, `D'`, `L'`, `R'`, `F'`, `B'`

**Examples:**
```javascript
"R U R' U'"
"F R U' R' U' R U R' F'"
"R2 U2 R U2 R2"
""  // Empty string (solved cube)
```

### `maxSolutions` (number, optional)

Maximum number of solutions to find before stopping the search.

**Type:** Integer  
**Range:** 1 to 999999  
**Default:** 20

**Examples:**
```javascript
{ maxSolutions: 1 }   // Find first solution only
{ maxSolutions: 100 } // Find up to 100 solutions
```

### `maxLength` (number, optional)

Maximum solution length in moves. Solutions longer than this will be ignored.

**Type:** Integer  
**Range:** 1 to 30 (recommended: 10-25)  
**Default:** 20

**Examples:**
```javascript
{ maxLength: 10 }  // Only solutions ≤10 moves
{ maxLength: 25 }  // Allow longer solutions
```

### `pruneDepth` (number, optional)

Search depth for the pruning table. Higher values find more solutions but are slower.

**Type:** Integer  
**Range:** 1 to 11  
**Default:** 8  
**Recommended:**
- Fast: 6 (quick search, fewer solutions)
- Standard: 8 (balanced)
- Thorough: 10-11 (slow, comprehensive)

**Examples:**
```javascript
{ pruneDepth: 6 }   // Fast search
{ pruneDepth: 11 }  // Complete search
```

**Warning:** Values >10 may take significantly longer.

### `rotation` (string, optional)

Apply a whole-cube rotation before solving.

**Type:** String  
**Allowed values:**
- X-axis: `"x"`, `"x2"`, `"x'"`
- Y-axis: `"y"`, `"y2"`, `"y'"`
- Z-axis: `"z"`, `"z2"`, `"z'"`
- No rotation: `""` (empty string)

**Default:** `""` (no rotation)

**Examples:**
```javascript
{ rotation: "y" }    // Rotate cube 90° on Y-axis
{ rotation: "y2" }   // Rotate 180° on Y-axis
{ rotation: "y'" }   // Rotate -90° on Y-axis
{ rotation: "x2" }   // Rotate 180° on X-axis
{ rotation: "" }     // No rotation
```

### `allowedMoves` (string, optional)

Restrict which moves the solver can use in solutions.

**Format:** Underscore-separated move list  
**Move notation:**
- Clockwise: `U`, `D`, `L`, `R`, `F`, `B`
- 180°: `U2`, `D2`, `L2`, `R2`, `F2`, `B2`
- Counter-clockwise: `U-`, `D-`, `L-`, `R-`, `F-`, `B-` (use `-` not `'`)

**Default:** `"U_U2_U-_R_R2_R-_F_F2_F-"` (URF faces, all turns)

**Examples:**
```javascript
// Only 90-degree clockwise turns
{ allowedMoves: "U_R_F_D_L_B" }

// URF faces with all turn types (default)
{ allowedMoves: "U_U2_U-_R_R2_R-_F_F2_F-" }

// All 18 moves (full move set)
{ allowedMoves: "U_U2_U-_D_D2_D-_L_L2_L-_R_R2_R-_F_F2_F-_B_B2_B-" }

// Only R and U moves
{ allowedMoves: "R_R2_R-_U_U2_U-" }

// No moves (will fail)
{ allowedMoves: "" }
```

**Note:** Prime moves (`'`) must be written as `-` in this parameter.

### `preMove` (string, optional)

Apply a move sequence before the search. The solver will find solutions that complete after this pre-move.

**Format:** Space-separated move notation (same as `scramble`)  
**Default:** `""` (no pre-move)

**Examples:**
```javascript
{ preMove: "U R" }        // Apply U R before solving
{ preMove: "R' F' R F" }  // Apply R' F' R F first
{ preMove: "" }           // No pre-move
```

### `moveOrder` (string, optional)

Override default move order constraints. By default, the solver follows these axis orders:
- X-axis: L → R → x
- Y-axis: U → D → y
- Z-axis: F → B → z

**Format:** `"<move1>~<move2>"` to reverse the constraint  
**Default:** `""` (use default ordering)

**Examples:**
```javascript
// Allow R before L (reverse default L→R)
{ moveOrder: "R~L", allowedMoves: "U_U2_U-_D_D2_D-_L_L2_L-_R_R2_R-_F_F2_F-_B_B2_B-" }

// Allow D before U (reverse default U→D)
{ moveOrder: "D~U", allowedMoves: "U_U2_U-_D_D2_D-_L_L2_L-_R_R2_R-_F_F2_F-_B_B2_B-" }

// No override
{ moveOrder: "" }
```

**Note:** Must include both moves in `allowedMoves`.

### `moveCount` (string, optional)

Limit how many times each move can appear in solutions.

**Format:** `"<move>:<count>_<move>:<count>_..."`  
**Count range:** 0 to 99  
**Default:** `""` (no limits)

**Examples:**
```javascript
// U max 2 times, R max 3 times
{ moveCount: "U:2_R:3" }

// No F moves, U max 1 time
{ moveCount: "F:0_U:1" }

// R and L each max 4 times
{ moveCount: "R:4_L:4" }

// No limits
{ moveCount: "" }
```

**Note:** 
- Use base move only (e.g., `U`, not `U2` or `U-`)
- Counts apply to all variations (e.g., `U:2` limits `U`, `U2`, `U'` combined)
- Moves not listed have no limit

### `class Solver2x2`

#### `async solve(scramble, options)` (Method)

Same as the standalone `solve()` function but as an instance method.

**Returns:** `Promise<string[]>`

#### `search(scramble, options)` (Method)

Starts an asynchronous search with event callbacks for streaming results.

**Parameters:**
- `scramble` (string, required): The scramble to solve
- `options` (object, optional):
  - **Solving parameters:** All parameters from `solve()` (see Parameter Reference)
  - **Callback functions:**
    - `onSolution` (function, optional): Called for each solution found
      - **Signature:** `(solution: string) => void`
      - **Argument:** Solution string (e.g., `"U R U' R'"`)
    - `onDone` (function, optional): Called when search completes successfully
      - **Signature:** `() => void`
    - `onError` (function, optional): Called if an error occurs
      - **Signature:** `(error: Error) => void`
      - **Argument:** Error object with message

**Returns:** `void` (use callbacks to receive results)

**Example:**
```javascript
const solver = new Solver2x2();
let count = 0;

solver.search("R U R' U'", {
  maxSolutions: 100,
  onSolution: (sol) => {
    count++;
    console.log(`Solution ${count}: ${sol}`);
  },
  onDone: () => console.log(`Found ${count} solutions`),
  onError: (err) => console.error('Error:', err)
});
```

#### `cancel()` (Method)

Cancels the current search and terminates the worker.

**Parameters:** None  
**Returns:** `void`

**Example:**
```javascript
const solver = new Solver2x2();
solver.search("R U R' U'", { onSolution: (s) => console.log(s) });

// Cancel after 1 second
setTimeout(() => solver.cancel(), 1000);
```

---

## Usage Examples

### Restricting Allowed Moves

Use the `allowedMoves` parameter to restrict the solver's move set. See [allowedMoves](#allowedmoves-string-optional) for full details.

```javascript
// Only 90-degree turns (no 180° or prime moves)
const solutions = await solve("R U R' U'", {
  allowedMoves: "U_R_F_D_L_B"
});

// URF faces only with all turn types (default)
const solutions = await solve("R U R' U'", {
  allowedMoves: "U_U2_U-_R_R2_R-_F_F2_F-"
});

// All 18 moves (complete move set)
const solutions = await solve("R U R' U'", {
  allowedMoves: "U_U2_U-_D_D2_D-_L_L2_L-_R_R2_R-_F_F2_F-_B_B2_B-"
});
```

**Note:** Prime moves (`'`) must be written as `-` (hyphen) in the `allowedMoves` string.

### Applying Cube Rotations

Apply whole-cube rotations before solving. See [rotation](#rotation-string-optional) for all valid values.

```javascript
// Rotate cube on Y-axis
const solutions = await solve("R U R' U'", {
  rotation: "y"
});

// 180-degree rotations
const solutions = await solve("R U R' U'", {
  rotation: "y2"
});

// Counter-clockwise rotation
const solutions = await solve("R U R' U'", {
  rotation: "y'"
});

// Other axes: "x", "x2", "x'", "z", "z2", "z'"
```

### Using Pre-moves

Apply a move sequence before the solver runs. See [preMove](#premove-string-optional).

```javascript
// Find solutions after applying U R
const solutions = await solve("R U R' U'", {
  preMove: "U R"
});

// Complex pre-move sequence
const solutions = await solve("F R U' R' U' R U R' F'", {
  preMove: "R' F' R F"
});
```

### Customizing Move Order

Override default move ordering rules. See [moveOrder](#moveorder-string-optional) for details.

```javascript
// Allow R→L instead of default L→R ordering
const solutions = await solve("R U R' U'", {
  moveOrder: "R~L",
  allowedMoves: "U_U2_U-_D_D2_D-_L_L2_L-_R_R2_R-_F_F2_F-_B_B2_B-"
});
```

**Default ordering rules:**
- X-axis: L → R → x
- Y-axis: U → D → y
- Z-axis: F → B → z

### Limiting Move Counts

Restrict how many times each move appears in solutions. See [moveCount](#movecount-string-optional).

```javascript
// U max 2 times, R max 3 times
const solutions = await solve("R U R' U'", {
  moveCount: "U:2_R:3"
});

// Forbid F moves entirely, limit U to 1 use
const solutions = await solve("R U R' U'", {
  moveCount: "F:0_U:1"
});
```

**Note:** Limits apply to all variations (e.g., `U:2` limits `U`, `U2`, and `U'` combined).

### Performance Tuning

Adjust search parameters for different speed/completeness trade-offs.

```javascript
// Fast search (quick results, may miss solutions)
const solutions = await solve("R U R' U'", {
  pruneDepth: 6,
  maxLength: 15,
  maxSolutions: 10
});

// Standard search (balanced)
const solutions = await solve("R U R' U'", {
  pruneDepth: 8,    // default
  maxLength: 20,    // default
  maxSolutions: 20  // default
});

// Thorough search (slower, finds more solutions)
const solutions = await solve("R U R' U'", {
  pruneDepth: 11,
  maxLength: 25,
  maxSolutions: 100
});
```

See [Performance Tips](#performance-tips) for recommendations.

## Performance Tips

- **Fast search**: `pruneDepth: 6`, `maxLength: 15`
- **Standard**: `pruneDepth: 8`, `maxLength: 20` (default)
- **Thorough**: `pruneDepth: 11`, `maxLength: 25` (slower)

Higher `pruneDepth` = more solutions but slower search.

## Files

- `2x2solver.js` - Simple JavaScript API (recommended)
- `worker.js` - Web Worker interface
- `solver.js` - Emscripten-generated JavaScript
- `solver.wasm` - WebAssembly binary
- `example.html` - Interactive examples

## Browser Support

Requires ES6 modules and Web Workers. Works in all modern browsers.

## License

See repository root for license information.
