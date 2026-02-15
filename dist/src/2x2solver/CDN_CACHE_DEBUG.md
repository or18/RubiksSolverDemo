# CDN Cache Debugging Guide

## 🔍 Problem: Old files still loading from CDN

If you're seeing old code even after purging CDN cache:

### Step 1: Verify CDN Files (Direct Links)

**Check current live versions on jsDelivr:**

1. **solver-helper.js** (should be ~255 lines with v1.1.0):
   ```
   https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js
   ```
   - Look for: `@version 1.1.0 (2026-02-15)`
   - Line count: ~255 lines
   - If you see 191 lines → OLD VERSION, wait 1-2 minutes

2. **worker_persistent.js**:
   ```
   https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/worker_persistent.js
   ```

3. **solver.js**:
   ```
   https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver.js
   ```

### Step 2: Force Browser Cache Clear

**Option A: Hard Refresh**
- Windows/Linux: `Ctrl + Shift + R` or `Ctrl + F5`
- Mac: `Cmd + Shift + R`

**Option B: DevTools Cache Disable**
1. Open DevTools (F12)
2. Network tab → Check "Disable cache"
3. Keep DevTools open while testing

**Option C: Incognito/Private Window**
- Chrome: `Ctrl + Shift + N`
- Firefox: `Ctrl + Shift + P`

### Step 3: Manual Cache Busting in Code

If using CDN in your HTML, add timestamp:

```html
<!-- ❌ OLD: Will use cached version -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>

<!-- ✅ NEW: Forces fresh download -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js?v=1771166366412"></script>
```

Generate new timestamp: `Date.now()` in browser console → `1771166366412`

### Step 4: Use Versioned Releases (Recommended)

Instead of `@main`, use specific version tags:

```html
<!-- Development (latest, may be cached) -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>

<!-- Production (stable, permanent cache) -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@v1.0.0/dist/src/2x2solver/solver-helper.js"></script>
```

## 🛠️ Troubleshooting Common Issues

### Issue 1: "Worker not found" error

**Symptom:**
```
Request URL: http://127.0.0.1:3000/src/test/worker_persistent.js
Status: 404 Not Found
```

**Cause:** Old 191-line `solver-helper.js` doesn't have CDN auto-detection

**Solution:**
1. Wait for CDN cache to clear (1-2 minutes after purge)
2. Hard refresh browser
3. Verify you're getting 255-line version (check console for version log)

### Issue 2: Mixed local/CDN loading

**Symptom:**
- `solver-helper.js` loads from CDN ✅
- `worker_persistent.js` tries to load from local path ❌

**Cause:** Browser cached the HTML page, not the JS

**Solution:**
1. Clear entire site data: DevTools → Application → Clear storage
2. Or use cache busting on HTML too: `test.html?v=123`

### Issue 3: Files still showing old content

**Symptom:**
- Purge script says "Cache cleared successfully"
- Direct URL still shows old code

**Cause:** jsDelivr multi-tier caching (can take up to 5 minutes)

**Solution:**
1. Wait 2-5 minutes after purge
2. Check GitHub repo directly to confirm new code is there
3. Use cache busting in the meantime: `?v=${Date.now()}`

## ✅ Verification Checklist

Before reporting cache issues:

- [ ] Ran `./purge-cdn-cache.sh` successfully
- [ ] Waited at least 2 minutes after purge
- [ ] Opened direct CDN URL in **new incognito window**
- [ ] Checked line count (should be ~255 for solver-helper.js)
- [ ] Searched for `@version 1.1.0` in the file
- [ ] Cleared browser cache with `Ctrl+Shift+R`
- [ ] Tried with DevTools "Disable cache" enabled
- [ ] Console shows `[Solver2x2Helper] v1.1.0 loaded from: ...`

## 🚀 Quick Test Commands

```bash
# 1. Purge CDN cache
cd dist/src/2x2solver
./purge-cdn-cache.sh

# 2. Wait 1-2 minutes, then verify
curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"
# Check: x-cache header (should be MISS, not HIT)

# 3. Check line count
curl "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js" | wc -l
# Expected: 255 (v1.1.0)
# If 191: OLD VERSION, wait longer or add ?v=timestamp
```

## 📞 Still Having Issues?

If files are still old after:
- ✅ Purging CDN
- ✅ Waiting 5 minutes
- ✅ Hard refresh
- ✅ Incognito mode

Then use explicit cache busting:

```html
<script>
const timestamp = Date.now(); // or hardcode a number
const script = document.createElement('script');
script.src = `https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js?v=${timestamp}`;
document.head.appendChild(script);
</script>
```

This forces CDN and browser to fetch fresh files.
