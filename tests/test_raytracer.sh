#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing raytracer..."
output=$(./build/raytracer dataset/ter.off /tmp/raytracer_test.ppm)
echo "$output"

if echo "$output" | grep -q "Saved to"; then
    if [ -f /tmp/raytracer_test.ppm ]; then
        echo "PASS: raytracer"
        exit 0
    fi
fi
echo "FAIL: raytracer"
exit 1