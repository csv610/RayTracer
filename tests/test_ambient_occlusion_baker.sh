#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing ambient_occlusion_baker..."
output=$(./build/ao_baker dataset/ter.off /tmp/ao_test.off 16)
echo "$output"

if echo "$output" | grep -q "AO baking complete"; then
    if [ -f /tmp/ao_test.off ]; then
        echo "PASS: ambient_occlusion_baker"
        exit 0
    fi
fi
echo "FAIL: ambient_occlusion_baker"
exit 1