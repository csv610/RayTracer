#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing accessibility_analysis..."
output=$(./build/accessibility_analysis dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "triangles are inaccessible"; then
    echo "PASS: accessibility_analysis"
    exit 0
else
    echo "FAIL: accessibility_analysis"
    exit 1
fi