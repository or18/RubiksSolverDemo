# CDN Testing Guide

This guide explains how to test the 2x2 solver library via CDN after pushing to GitHub.

## Quick Start

1. **Commit and push files to GitHub:**
   ```bash
   git add .gitignore dist/
   git commit -m "Add 2x2 solver library for CDN distribution"
   git push origin main
   ```

2. **Wait 1-2 minutes** for CDN to cache (jsDelivr updates from GitHub)

3. **Test CDN access:**
   - Open: `dist/cdn-test.html?cdn=jsdelivr`
   - Or: `dist/cdn-test.html?cdn=githubRaw`
   - Or: `dist/cdn-test.html` (local by default)

## CDN URLs

After pushing to GitHub, the library will be available at:

### jsDelivr (Recommended)
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/2x2solver.js
```

**Features:**
- ✅ Fast global CDN
- ✅ Automatic caching
- ✅ CORS enabled
- ✅ Versioning support (`@main`, `@v1.0.0`, `@commit-hash`)

### GitHub Raw
```
https://raw.githubusercontent.com/or18/RubiksSolverDemo/main/dist/src/2x2solver/2x2solver.js
```

**Features:**
- ✅ Direct from GitHub
- ⚠️ Slower (no CDN)
- ⚠️ Rate limited
- ⚠️ CORS may have issues

## Usage via CDN

### Option 1: Direct Import (jsDelivr)

```html
<!DOCTYPE html>
<html>
<head>
  <script type="module">
    import { solve } from 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/2x2solver.js';
    
    const solutions = await solve("R U R' U'");
    console.log(solutions);
  </script>
</head>
</html>
```

### Option 2: With Import Map

```html
<!DOCTYPE html>
<html>
<head>
  <script type="importmap">
  {
    "imports": {
      "2x2solver": "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/2x2solver.js"
    }
  }
  </script>
  <script type="module">
    import { solve } from '2x2solver';
    
    const solutions = await solve("R U R' U'");
    console.log(solutions);
  </script>
</head>
</html>
```

## Testing Checklist

Run `cdn-test.html` and verify:

- [ ] **Test 1: Module Import** - Should show ✅ with green status
- [ ] **Test 2: Basic Solve** - Should find solutions for scramble
- [ ] **Test 3: Worker Functionality** - Should stream solutions via callbacks

## Known Issues & Solutions

### Issue: Module not found (404)

**Cause:** Files not yet pushed or CDN not updated

**Solution:**
1. Verify files are pushed: `git ls-tree -r main --name-only | grep dist`
2. Wait 1-2 minutes for jsDelivr to update
3. Purge cache: https://www.jsdelivr.com/tools/purge

### Issue: CORS error

**Cause:** Worker files loaded from different origin

**Solution:**
- Use jsDelivr (has proper CORS headers)
- Or use importScripts with Blob URL (already implemented in 2x2solver.js)

### Issue: Worker not working

**Cause:** Worker relative paths don't resolve correctly from CDN

**Solution:**
The library uses `import.meta.url` to resolve worker paths dynamically. This should work automatically.

If issues persist, use the local copy or configure base URL:
```javascript
// Manual worker URL (rarely needed)
const solver = new Solver2x2();
// Library handles this automatically
```

## Versioning

### Latest (main branch)
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/2x2solver.js
```

### Specific commit
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@<commit-hash>/dist/src/2x2solver/2x2solver.js
```

### Tag/Release (recommended for production)
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@v1.0.0/dist/src/2x2solver/2x2solver.js
```

## Performance Notes

- **First load:** ~500KB (WASM download)
- **Subsequent loads:** Cached by browser
- **jsDelivr:** Global CDN, very fast
- **GitHub raw:** Slower, no caching

## Next Steps

After successful CDN testing:

1. **Create GitHub Release** with version tag (`v1.0.0`)
2. **Update dist/README.md** with confirmed CDN URLs
3. **Test in production environment** (timer apps, etc.)
4. **Document any edge cases** found during testing

## Support

If CDN loading fails, users can always fall back to local installation:
1. Download `dist/src/2x2solver/` directory
2. Import locally: `import { solve } from './2x2solver/2x2solver.js'`
