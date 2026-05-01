#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing symmetry_detection..."
output=$(./build/symmetry_detection dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Plane 0:"; then
    echo "PASS: symmetry_detection"
    exit 0
else
    echo "FAIL: symmetry_detection"
    exit 1
fi