**Tools Utilities — Minimal API Reference**

This file documents a concise subset of the helper functions available in `dist/src/utils/tools.js`. Keep examples small — these are intended for quick tests and typical worker/browser usage.

**buildRestFromArray**: Input & format
- Input: `restArray` — Array of strings. Each element is a move token; apostrophe (`'`) notation is allowed (e.g. `"R'"`).
- Returns: Normalized `rest` string where primes are hyphens (`-`) and tokens are joined by `_` (e.g. `R_R2_R-`).

**buildRestFromArray**: Output
- Output: `string` (empty string if input empty).

**buildRestFromArray**: Example
- Node: `const rest = t.buildRestFromArray(['R','R2',"R'"]); // -> "R_R2_R-"`

**buildRestFromArray**: Edge cases / Limitations
- Non-array input: `buildRestFromArray` throws if the argument is not an `Array`.
- Empty or all-empty tokens: `[]` or `["", null]` returns an empty string `""`. Note: the solver requires a non-empty `rest` — validate with `validateRest` before using in solver calls.
- Non-string elements: non-strings are coerced to empty and filtered out; if that removes all tokens the function returns `""`.
- Tokens must not contain underscores (`_`) or internal spaces. Underscore is the internal separator and will produce invalid tokens; internal spaces are not removed and will cause validation to fail.
- Invalid move tokens: if any token is not in the allowed move set for the `puzzleType` (see `validateRest`), `buildRestFromArray` will throw. Example failing input: `['R','Foo']` -> throws.
- Duplicates: duplicate tokens are preserved (no deduplication is performed).
- Prime notation: apostrophe (`'`) is accepted and converted to hyphen (`-`) automatically; the internal validators expect hyphen form. Prefer supplying canonical hyphen-form when persisting or sharing `rest` strings.

Recommended usage: provide a clean array of canonical move tokens (strings without underscores or internal spaces). When in doubt, call `validateRest` on the returned string before using it with solver APIs.

**buildMcvFromObject**: Input & format
- Input: `rest` (array or string) and `mapping` object: `{ move: count, ... }`.
- `move` may use apostrophe in the input (converted to hyphen internally). Counts must be non-negative integers.

**buildMcvFromObject**: Output
- Output: `string` with entries `Move:count` joined by `_`, using hyphen for primes (e.g. `R:1_R-:2`).

**buildMcvFromObject**: Example
- Node: `const mcv = t.buildMcvFromObject(['R','R2',"R'"], { R:1, "R'":2 }); // -> "R:1_R-:2"`

**buildMcvFromObject**: Edge cases / Limitations
- `mapping` may be `null` or `undefined` — in that case the function returns an empty string `""` (useful when optional). However, if provided it must be a plain object; arrays will cause a thrown error.
- Counts: numbers or numeric strings are accepted, but must be non-negative integers. Negative, fractional, or non-numeric counts will cause an error.
- Move keys: mapping keys may use apostrophe (`'`) notation; they are converted to hyphen (`-`) before validation. The move must exist in `rest` after normalization or an error is thrown.
- `rest` must be either an array of move tokens or a string; `null`/non-string/non-array `rest` will cause an error. When `rest` is an array, it is normalized by `buildRestFromArray` (apostrophes converted to hyphens).
- Empty mapping `{}` returns `""`.

Recommended usage: treat `mapping` as optional; when present, supply clean token keys (or apostrophe-form which will be normalized) and integer counts.

**buildMavFromPairs**: Input & format
- Input: `rest` (array or string) and `pairs` — an array of pairs: `[[left, right], ...]`.
- `left` may be empty string (`''`) to indicate `EMPTY`. Both `left` and `right` may be provided with apostrophe notation (converted to hyphen).
- Example pair: `['', 'R2']` -> `EMPTY~R2`.

**buildMavFromPairs**: Output
- Output: `string` with `left~right` entries joined by `|`, using hyphen for primes (e.g. `R~R2|EMPTY~R2`).

**buildMavFromPairs**: Example
- Node: `const mav = t.buildMavFromPairs(['R','R2',"R'"], [['R','R2'], ["R'","R"], ['', 'R2']]); // -> "R~R2|R-~R|EMPTY~R2"`

**buildMavFromPairs**: Edge cases / Limitations
- `pairs` must be an array of two-element arrays. Non-array entries or arrays not of length 2 will throw.
- `left` may be an empty string (`''`) or `null` — both are treated as `EMPTY` in the resulting MAV entry. The literal string `"EMPTY"` may also be supplied for `left`.
- `right` must be a valid token present in `rest`. An empty string (`''`) or the literal `"EMPTY"` on the right-hand side will cause an error.
- Duplicate pairs are removed (deduplicated) by `buildMavFromPairs`.
- Apostrophe (`'`) notation in pair tokens is accepted and converted to hyphen (`-`) before validation.
- `rest` must be an array or a string; `null` or other types will throw.

Recommended usage: supply `rest` and `pairs` as clean arrays of strings. Use `validateMav` to verify generated `mav` strings before consuming them in solver APIs.

**buildCrestFromArray**: Input & format
- Input: `pairs` — either array of strings or array of two-element arrays. Examples:
  - `['x', 'x y']`
  - `[ ['x','y'], ['z2','y'] ]`
- This function is `async` and may call the embedded C++ helper to canonicalize single-token center offsets.

**buildCrestFromArray**: Output
- Output: `string` of pipe-separated entries where each entry is `row_col` using hyphens for primes and underscores between row/col (e.g. `x_y|z2_y`). Returns empty string on empty input.
- Note: must `await` the result: `const crest = await t.buildCrestFromArray(...);`

**buildCrestFromArray**: Edge cases / Limitations
- Input must be an array. Passing `null` or other non-array types will throw.
- Two accepted forms: array of two-element arrays ([[row, col], ...]) or array of strings (single token strings or "row col"). Strings with more than two space-separated parts will cause an error.
- Empty string token (`''`) becomes `EMPTY_EMPTY`.
- Single-token strings delegate to the C++ helper via `getCenterOffset` to canonicalize the center offset; if the native module is unavailable this step may throw — callers should be prepared to handle or skip such failures in environments without the compiled helper.
- Duplicate crest entries are removed (deduplicated) while preserving order.
- Apostrophe (`'`) notation in inputs is accepted and converted to hyphen (`-`) in outputs (e.g. `x' y'` -> `x-_y-`).

Recommended usage: prefer passing `[ ['row','col'], ... ]` arrays when possible for deterministic, synchronous behavior. When using single-token strings that require C++ canonicalization, ensure the runtime can load the native module or catch errors.

**getCenterOffset**: Input & format
- Input: `premove` (string) and optional `puzzleType`.
- The JS wrapper preprocesses the input with `splitRotationAlg(scr_fix(...)).rotation` before calling the C++ function — you may pass human-friendly apostrophe notation; the wrapper ensures the C++ function receives a canonical rotation string.

**getCenterOffset**: Output
- Output: Promise resolving to the canonical center-offset string produced by the C++ helper: space-separated family parts using hyphen for primes (e.g. `z2 y2`, `x y`).
- Note: `buildCrestFromArray` converts the C++ output into the `row_col` form (spaces -> underscores, primes -> hyphens) automatically; callers that build crest entries manually should convert spaces to underscores when needed.
- Usage: `const c = await t.getCenterOffset('x2');`

**invertAlg**: Input & format
- Input: `algStr` (string), optional `puzzleType` (default `'3x3'`). The function normalizes lines/comments using `scr_fix2` before processing.

**invertAlg**: Output
- Output: Promise resolving to the reversed algorithm string. The function preserves `//` comments on lines and returns a string terminated with a newline. If input normalizes to empty, the function returns `undefined`.

**invertAlg**: Example
- Node: `const rev = await t.invertAlg("R U R'"); // -> reversed alg string` 

**mirrorAlg**: Input & format
- Input: `algStr` (string), optional `puzzleType` (default `'3x3'`). Accepts multi-line strings with `//` comments; uses `scr_fix2` for normalization.

**mirrorAlg**: Output
- Output: Promise resolving to the mirrored algorithm string. Preserves line comments and returns a string terminated with a newline. If input normalizes to empty, the function returns `undefined`.

**mirrorAlg**: Example
- Node: `const m = await t.mirrorAlg("R U R'"); // -> mirrored alg string` 

**rotateAlg**: Input & format
- Input: `scramble` (string), `rotation_scramble` (string), optional `puzzleType` (default `'3x3'`). Both inputs are normalized with `scr_fix` before being passed to the native helper.

**rotateAlg**: Output
- Output: Promise resolving to the rotated algorithm string produced by the compiled helper (string). Throws if the native `AlgRotation` function is unavailable.

**rotateAlg**: Example
- Node: `const out = await t.rotateAlg('R U R\'', 'x2'); // -> rotated alg string` 

**rotateAlg**: Edge cases / Limitations
- Both arguments must be strings. Passing `null`/`undefined` will throw during internal normalization (`scr_fix`).
- An empty `rotation_scramble` is accepted and the function returns a string (effectively no rotation).

**splitRotationAlg**: Input & format
- Input: a single scramble string and optional `puzzleType` (default `'3x3'`). The function normalizes the input via `scr_fix` and calls the native converter.

**splitRotationAlg**: Output
- Output: Promise resolving to an object `{ rotation: string, scramble: string }` where `scramble` is the normalized scramble and `rotation` is the extracted rotation token/string.

**splitRotationAlg**: Example
- Node: `const parts = await t.splitRotationAlg('R U R\' x2'); // -> { scramble: 'R U R\'', rotation: 'x2' }`

**splitRotationAlg**: Edge cases / Limitations
- Input must be a string. Passing `null` or `undefined` will cause a runtime error (the implementation calls `scr_fix` which expects a string).
- Empty or unrecognized-token inputs return `{ rotation: '', scramble: '' }` rather than throwing; callers should treat empty `rotation` as "no rotation".

**generateTwoPhaseInput**: Input & format
- Input: `scramble` (string) and optional `puzzleType` (default `'3x3'`). This function calls into the compiled `ScrambleToState` helper.
- Note: The output format is the 54-character cube state string used by common 3x3 two-phase solvers. The function currently accepts a `puzzleType` argument but the produced 54-char state is meaningful only for 3x3 puzzles.

**generateTwoPhaseInput**: Output
- Output: Promise resolving to the 54-character cube state string (string). Throws if the native `ScrambleToState` helper is unavailable.

**generateTwoPhaseInput**: Example
- Node: `const state54 = await t.generateTwoPhaseInput('R U R\'' ); // -> 54-char state string (3x3 only)`

**generateTwoPhaseInput**: Edge cases / Limitations
- Input must be a string. Passing `null`/`undefined` will throw; empty string inputs produce a 54-character state string (the compiled helper still returns a 54-char state for 3x3 semantics).
- The `puzzleType` argument is accepted but the output is meaningful only for 3x3; calling with other `puzzleType` values (e.g. `'2x2'`) will still return a 54-char string but that result is not meaningful for non-3x3 puzzles.

Recommended usage: validate that `scramble` is a non-empty string before calling. Treat the returned 54-character string as valid only for 3x3 use-cases.

**scrambleFilter**: Input & format
- Input: single `scramble` string. This function uses `scr_fix`/`scr_fix2` internally and returns a normalized scramble string suitable for downstream parsing.

**scrambleFilter**: Output
- Output: normalized scramble string (string). Use this before feeding scrambles to other helpers.

**scrambleFilter**: Edge cases / Limitations
- Input must be a string. Passing `null` or calling without an argument will throw (internal normalization functions expect strings).
- By default `removeComments` is `true` and `scrambleFilter` removes `//` style comments and blank lines (`scr_fix`). If `removeComments` is `false`, comments are preserved and returned (via `scr_fix2`).
- A comment-only line becomes an empty string when `removeComments` is `true`.
- `scrambleFilter` normalizes whitespace and tabs; tokens like `RUR'F2` are expanded to `R U R' F2` where recognized tokens match the supported token list.
- Unknown tokens (not in the token list) are left as-is (e.g. `FOO` remains `FOO`).
- Very long inputs are handled (the function is synchronous and operates on strings), but callers should avoid extremely large strings to prevent memory issues in constrained environments.

Recommended usage: ensure you pass a string. Use the default `removeComments=true` for sanitized scrambles; set `removeComments=false` only if you need to preserve comments for human-facing output.

**Examples — Node quick test**
```javascript
const t = require('../tools.js');
(async ()=>{
  const rest = t.buildRestFromArray(['R','R2',"R'"]);
  const mcv = t.buildMcvFromObject(rest, { R:1, "R'":2 });
  const mav = t.buildMavFromPairs(rest, [['R','R2'], ['', 'R2']]);
  const crest = await t.buildCrestFromArray(['x','x y']);
  console.log({ rest, mcv, mav, crest });
})();
```

See the practical test runner: [dist/src/utils/test_tools.js](dist/src/utils/test_tools.js)

**Examples — Worker / Browser**
```javascript
// inside a worker that loads tools.js as a script (non-ESM)
self.onmessage = async (e) => {
  const t = self.__TOOLS_UTILS_EXPORTS__;
  const rest = t.buildRestFromArray(['R','R2',"R'"]);
  const crest = await t.buildCrestFromArray(['x']);
  postMessage({ rest, crest });
};
```

**Notes / Gotchas**
- Validators (internal) expect hyphen (`-`) for primes. The builder helpers accept apostrophe (`'`) in inputs and convert them to hyphens for outputs and for C++ calls. When storing or passing parameters between services, prefer hyphen notation to avoid confusion.
- `buildCrestFromArray` and `getCenterOffset` are asynchronous and must be awaited.
- `left` in a `mav` pair may be `EMPTY` (or `''` in the input array) but the right-hand side must be a valid token present in `rest`.

