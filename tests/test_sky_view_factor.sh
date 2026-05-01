#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing sky_view_factor..."
output=$(./build/sky_view_factor dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Sky View analysis complete"; then
    echo "PASS: sky_view_factor"
    exit 0
else
    echo "FAIL: sky_view_factor"
    exit 1
fi