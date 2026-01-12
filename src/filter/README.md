# Filter.js - Web Worker Edition Quick Start Guide

## 🚀 Start in 3 Steps

### 1. Start HTTP Server

```bash
cd /workspaces/RubiksSolverDemo/src/filter
python3 -m http.server 8080
```

### 2. Open in browser

```
http://localhost:8080/filter_worker_example_enhanced.html
```

### 3. Test execution

1. Click the “Load Sample Data” button
2. Click the “Run Test with Current Parameters” button
3. Check the results!

---

## ✅ Verification

### Test with Node.js

```bash
# Web Worker integration test
node test_worker_integration.js

# Expected output:
# ✅ All tests PASSED! Web Worker integration is working correctly.
```

---

## 📋 Important Points

### ✅ DO (Recommended)
- Access via HTTP server (`http://localhost:8080/...`)
- Use modern browsers (Chrome, Firefox, Edge)
- Check logs in developer tools (F12 key)

### ❌ DON'T (Not Recommended)
- Open files directly (`file:///...`) → Web Worker will not work
- Use old browsers → Web Worker API may not be supported

---

## 🔧 Technical Specifications

### Web Worker Communication

**Sent Data (HTML → Worker):**
```javascript
{
  solutions: string[],      // List of solutions
  minClusterSize: number,   // Minimum cluster size
  mergeThreshold: number    // Merge threshold (threshold)
}
```

**Received Data (Worker → HTML):**
```javascript
{
  labels: [
    { school: string, label: string, rank: number | null }
  ],
  stats: {
    total: number,
    labeled: number,
    unlabeled: number,
    representative: number,
    shortest: number,
    member: number,
    schools: number
  }
}
```

---

## 📚 Detailed Documentation

- **docs/USAGE_HTML_TOOL.md** - Detailed usage instructions for HTML interface
- **docs/DEVELOPER_GUIDE.md** - Developer guide with algorithm details and implementation examples
- **docs/PARAMETERS.md** - Complete parameter reference and configuration guide
- **docs/UNUSED_FUNCTIONS_REPORT.md** - Unused functions report (for code cleanup)

---
---

**Last Updated**: 2026-01-12  
**Version**: Web Worker Edition