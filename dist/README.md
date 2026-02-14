# Rubik's Cube Solver Libraries

WebAssembly-based Rubik's Cube solvers with simple JavaScript APIs.

## Overview

This library provides high-performance cube solvers compiled to WebAssembly, offering both Promise-based and event-driven APIs for various cube puzzles.

## Installation

### Option 1: ES6 Module (Recommended)

```javascript
import { solve } from './src/2x2solver/2x2solver.js';

const solutions = await solve("R U R' U'");
console.log(solutions);
```

### Option 2: CDN (Coming Soon)

```javascript
// Future CDN support
import { solve } from 'https://cdn.example.com/rubiks-solver/2x2solver.js';
```

### Option 3: Local Files

Download the solver directory and include files in your project.

## Browser Requirements

- ES6 module support
- Web Workers
- WebAssembly

All modern browsers (Chrome, Firefox, Safari, Edge) are supported.

---

## 2x2x2 Cube Solver

Optimal solver for 2x2x2 Rubik's Cube (Pocket Cube).

### Basic Usage

```javascript
import { solve } from './src/2x2solver/2x2solver.js';

// Simple solve
const solutions = await solve("R U R' U'");

// With options
const solutions = await solve("R U R' U'", {
  maxSolutions: 10,
  maxLength: 15,
  pruneDepth: 8
});
```

### API Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `scramble` | string | *(required)* | Scramble sequence (e.g., `"R U R' U'"`) |
| `maxSolutions` | number | `20` | Maximum number of solutions to find |
| `maxLength` | number | `20` | Maximum solution length in moves |
| `pruneDepth` | number | `8` | Search depth (1-11, higher = more thorough) |
| `rotation` | string | `""` | Whole-cube rotation (`"y"`, `"z"`, `"x"`, etc.) |
| `allowedMoves` | string | `"U_U2_U-_R_R2_R-_F_F2_F-"` | Allowed move set (underscore-separated) |
| `preMove` | string | `""` | Pre-move sequence |
| `moveOrder` | string | `""` | Move order constraints |
| `moveCount` | string | `""` | Move count limits (e.g., `"U:2_R:3"`) |

### Event-Based API

For streaming results:

```javascript
import { Solver2x2 } from './src/2x2solver/2x2solver.js';

const solver = new Solver2x2();
solver.search("R U R' U'", {
  onSolution: (sol) => console.log('Found:', sol),
  onDone: () => console.log('Complete')
});
```

### Performance Modes

| Mode | `pruneDepth` | `maxLength` | Use Case |
|------|--------------|-------------|----------|
| Fast | `6` | `15` | Quick results |
| Standard | `8` | `20` | Balanced (default) |
| Thorough | `10-11` | `25` | Complete search |

### Common Use Cases

**Restrict to specific faces (e.g., RU only):**
```javascript
await solve("R U R' U'", {
  allowedMoves: "R_R2_R-_U_U2_U-"
});
```

**Apply cube rotation:**
```javascript
await solve("R U R' U'", {
  rotation: "y"  // Rotate on Y-axis
});
```

**Find optimal solutions only:**
```javascript
await solve("R U R' U'", {
  maxLength: 10,
  maxSolutions: 1
});
```

### Files

- `2x2solver.js` - Main API (Promise-based)
- `worker.js` - Web Worker interface
- `solver.wasm` - WebAssembly binary
- `example.html` - Interactive examples

📖 **[Full Documentation](./src/2x2solver/README.md)** - Complete parameter reference, advanced options, and detailed examples.

---

## Other Solvers

*(Coming soon)*

- 3x3x3 Cross Solver
- 3x3x3 EOCross Solver
- 3x3x3 XCross Solver
- 3x3x3 XXCross Solver
- F2L Pairing Solver

---

## License

See repository root for license information.

## Contributing

Contributions welcome! Please see the main repository for guidelines.
