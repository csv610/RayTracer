#!/bin/bash
set -e
cd "$(dirname "$0")/.."
echo "Testing ray_traced_slicer..."
output=$(./build/ray_traced_slicer dataset/ter.off -o /tmp/slice -l 10 -r 64)
echo "$output"
if [ -f /tmp/slice_0.ppm ]; then
    echo "PASS: ray_traced_slicer"
    exit 0
fi
echo "FAIL: ray_traced_slicer"
exit 1
