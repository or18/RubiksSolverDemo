# How to use Filter.js parameter tuning tool
**User Guide**: This document explains how to use the HTML-based parameter tuning interface.

**Related Documentation**:
- **PARAMETERS.md** - Complete parameter reference and descriptions
- **DEVELOPER_GUIDE.md** - Algorithm details and advanced configuration

---
## 📁 File

-`filter_worker_example_enhanced.html` -Parameter tuning tool (Web Worker version)
-`filter.js` -Filtering engine (Web Worker compatible, all parameters can be set externally)
-`sol.txt`, `sol2.txt` -Test data

## ✨ New features

**All parameters are now adjustable from the UI!**

You can change the following parameters in real time:
- School classification parameters (threshold, minClusterSize, maxClusterSize)
- Subgroup division parameters (subgroupThreshold, minSubgroupSize)
- Scoring weight distribution (lengthWeight, rankWeight, sizeWeight)
- QTM bonus settings (qtmSubgroup, qtmCase1, qtmFinal)
- Weight distribution for similarity calculation (endFaceWeight, endSequenceWeight)

**All parameters are sent to the Web Worker as a single `config` object.**

## 🚀 How to use

### 1. Open the HTML file

#### Method A: Local HTTP Server (Recommended)

```bash
cd /workspaces/RubiksSolverDemo/src/filter
python3 -m http.server 8080
```

  Open in browser:
```
http://localhost:8080/filter_worker_example_enhanced.html
```

**important**: Since it uses a Web Worker, access via an HTTP server is **required**.

#### Method B: Open the file directly (Not recommended)

```bash
# Open the HTML file in the browser
xdg-open filter_worker_example_enhanced.html
# Or
"$BROWSER" filter_worker_example_enhanced.html
```

**WARNING**: For method B, the Web Worker may not work (CORS limitations). Be sure to use it via an HTTP server.

---

### 2. Load sample data

Click the "Load sample data" button to automatically enter test data.

Or enter the solution manually in the text area:

```
1: R U R' U'
2: F R U' R' F'
3: R U2 R' U'
```

---

### 3. Adjust parameters

All parameters are now adjustable from the UI!

#### 1. School classification parameters
- **Threshold (school distance threshold)**: 2.0~10.0
  - Small: School is subdivided
  - Large: Schools are integrated.
  - Recommended: 100 solutions=4, 500 solutions=5, 1000 solutions=6
  
- **Min Cluster Size (minimum school size)**: 2~15
  - Minimum number of members recognized as a school
  - Small: More schools, Large: Selected schools

- **Max Cluster Size (maximum school size)**: 10~100
  - Maximum number of members in a single school
  - If exceeded, the school is split (for large samples)

#### 2. Subgroup division parameters
- **Subgroup Threshold**: 1.5~4.0
  - Threshold for dividing subgroups within a school
  - Small: Subgroups are subdivided, Large: Subgroups are integrated

- **Min Subgroup Size**: 2~5
  - Minimum subgroup size required for labeling
  - Recommended: 3 (stability-focused), 2 (diversity-focused)

#### 3. Scoring weight distribution (%)
- **Length Weight (weight for solution length)**: 20~60%
  - Degree to favor shorter solutions
- **Rank Weight (weight for school rank)**: 10~40%
  - Degree to favor large schools
- **Size Weight (weight for school size)**: 5~30%
  - Degree to favor school size

#### 4. QTM bonus settings
- **Subgroup QTM Multiplier**: 0~1.0
  - Degree to emphasize QTM when selecting the best solution within a subgroup
- **Case1 QTM Multiplier**: 0.5~3.0
  - Degree to emphasize QTM when no subgroups are formed
- **Final Ranking QTM Multiplier**: 0~0.5
  - Fine-tuning of QTM at final ranking (keep small)

#### 5. Weight distribution for similarity calculation
- **End Similarity - Face Weight**: 20~60%
  - Weight of face similarity in school classification
- **Chunk Similarity - Jaccard Weight**: 20~60%
  - Weight of Jaccard similarity in subgroup division

**All parameters are sent to the Web Worker and reflected in the results in real time.**

---

### 4. Use Presets

The following five presets are available:

1. **Default Setting** - threshold=4 (recommended)
2. **Strict Mode** - threshold=3 (subdivide schools)
3. **Relaxed Mode** - threshold=6 (integrate schools)
4. **Quality Focused** - threshold=4
5. **QTM Focused** - threshold=4

---

### 5. Run Tests

Click the "Run Test with Current Parameters" button to:

1. Process the entered solutions with filter.js
2. Display statistics
   - Total
   - Labeled
   - Schools
   - Representatives
   - Alternatives
   - Perfect Score
3. Display processing time and parameters
4. Display result list

---

### 6. Check Results

#### Change Sort Order
- By Rank (default)
- By School
- By Original Order
- By QTM

#### Filter
- All
- Labeled Only
- Representatives Only
- Top 15

#### How to Read Results

Each row displays the following information:
- **Index (#)**: Original solution number (input order)
- **Rank**: Recommended rank
- **School**: School name (e.g., School_1)
- **Label**: Label (Representative, Alternative, etc.)
- **Score**: Base score (0-100)
- **Adj**: Adjusted score (including bonuses)
- **HTM/QTM**: Half Turn Metric / Quarter Turn Metric
  - HTM: Count all turns as 1 (180-degree turns also count as 1)
  - QTM: Count 180-degree turns as 2
  - The ratio (HTM/QTM) indicates **turning efficiency**
    - Close to 1.0: Few 180-degree turns, efficient (green)
    - 0.7-0.85: Moderate efficiency (yellow-green)
    - Below 0.7: Many 180-degree turns (gray)
- **Solution**: Solution steps

Color coding:
- 🟢 Green left border: Rank 1
- 🔵 Blue left border: Rank 2-5
- 🔘 Faded display: Unlabeled

---

### 7. Export Parameters

Click the "Export Parameters" button to save the current settings in JSON format.

---

## 🧪 Tested

The following tests have been confirmed to work:

```bash
# Web Worker integration test
node test_worker_integration.js

# Expected output:
# ✅ All tests PASSED! Web Worker integration is working correctly.
```

**Test results:**
- ✅ Web Worker communication works correctly
- ✅ Labels structure is correct (school, label, rank)
- ✅ Statistics are calculated correctly
- ✅ In a sample of 10 solutions, 3 are labeled and 1 school is detected

---

## ⚠️ Limitations

1. **HTTP Server Required**: To use Web Workers, always access via an HTTP server
   - Web Workers do not work with the file:// protocol
   - Use `python3 -m http.server 8080`

2. **Parameter Restrictions**: In the Web Worker version, only the following parameters can be changed dynamically
   - `threshold` (mergeThreshold)
   - `minClusterSize`
   - To adjust other parameters, you need to edit filter.js directly

3. **Browser Compatibility**: Tested on modern browsers (Chrome, Firefox, Edge)
   - Requires browsers that support the Web Worker API

4. **Score Display Limitations**: Detailed scores (score, adjustedScore, qtm) are not displayed in the Web Worker version
   - Because these details are not included in the worker response
   - Rank, school, and label are displayed correctly
---

## 📚 Related Documents

- `DEVELOPER_GUIDE.md` - Developer Guide
- `test_comprehensive.js` - Comprehensive Test Script

---

## 🐛 Troubleshooting

### Issue: Nothing happens when the button is pressed

**Solution**:
1. **Check if accessed via HTTP server** (most important)
   - Ensure the URL is `http://localhost:8080/...`
   - Web Workers do not work with `file:///...` protocol
2. Check for errors in the browser's developer tools (F12 key)
3. Verify that filter.js is loaded correctly (Network tab)
4. Ensure solutions are entered in the text area

### Issue: "Worker error" is displayed

**Solution**:
1. Check if the HTML file is in the same directory as filter.js
2. Verify that the HTTP server is running correctly
3. Check the browser console for detailed error messages

### Issue: "sol.txt" cannot be loaded

**Solution**:
1. Open the HTML file via an HTTP server
2. Or use the built-in sample by clicking the "Load Sample Data" button

### Issue: Results are not displayed

**Solution**:
1. Check the format of the entered solutions (one solution per line)
2. Check for error messages in the browser console
3. Enter at least 3 solutions
4. Verify that the processing message is displayed (confirm Web Worker startup)
---

## 💡 Usage Examples

### Example 1: Test with Default Settings

1. Click "Load Sample Data"
2. Click "Run Test with Current Parameters"
3. Check the results

### Example 2: Refine Schools

1. Select the "Strict Mode" preset
2. Confirm that the Threshold is set to 3.0
3. Click "Run Test with Current Parameters"
4. Confirm that the number of schools increases

### Example 3: Test with Custom Data

1. Select your own text file using "Load from File"
2. Adjust the Threshold slider
3. Click "Run Test with Current Parameters"
4. Check the results and adjust parameters as needed

---

**Last Updated**: 2026-01-12