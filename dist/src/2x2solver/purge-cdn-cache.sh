#!/bin/bash

# jsDelivr CDN Cache Purge Script
# Usage: ./purge-cdn-cache.sh

echo "🔄 Purging jsDelivr CDN cache for 2x2 solver files..."

BASE_URL="https://purge.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver"

# Purge all solver files
files=(
    "worker_persistent.js"
    "solver.js"
    "solver.wasm"
)

for file in "${files[@]}"; do
    echo "Purging: $file"
    response=$(curl -s "${BASE_URL}/${file}")
    
    # Check if response contains "finished" status (allow whitespace)
    if echo "$response" | grep -q 'status.*finished'; then
        echo "✅ $file - Cache cleared successfully"
    else
        echo "❌ $file - Failed"
        echo "   Response: $response"
    fi
done

echo ""
echo "✨ Done! Wait 10-30 seconds for propagation."
echo "   Then reload cdn-test.html with Ctrl+Shift+R"
