#!/bin/bash
set -e
cd "$(dirname "$0")/.."
echo "Testing symmetry_detection..."
output=$(./build/symmetry_detection dataset/ter.off)
echo "$output"
if echo "$output" | grep -q "Best symmetry plane found:"; then
    echo "PASS: symmetry_detection"
    exit 0
fi
echo "FAIL: symmetry_detection"
exit 1
