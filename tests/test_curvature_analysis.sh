#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing curvature_analysis..."
output=$(./build/curvature_analysis dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Loading mesh:"; then
    echo "PASS: curvature_analysis"
    exit 0
else
    echo "FAIL: curvature_analysis"
    exit 1
fi