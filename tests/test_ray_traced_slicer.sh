#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing ray_traced_slicer..."
output=$(./build/ray_traced_slicer dataset/ter.off 4 16 2>&1)
echo "$output"

if echo "$output" | grep -q "Slicing complete"; then
    echo "PASS: ray_traced_slicer"
    exit 0
fi
echo "FAIL: ray_traced_slicer"
exit 1