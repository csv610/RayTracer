#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing depth_map..."
output=$(./build/depth_map dataset/ter.off /tmp/depth_test.ppm)
echo "$output"

if echo "$output" | grep -q "Depth map saved to"; then
    if [ -f /tmp/depth_test.ppm ]; then
        echo "PASS: depth_map"
        exit 0
    fi
fi
echo "FAIL: depth_map"
exit 1