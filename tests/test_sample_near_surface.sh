#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing sample_near_surface..."
output=$(./build/sample_near_surface dataset/ter.off 100 1 /tmp/sample_surface_test.off)
echo "$output"

if echo "$output" | grep -q "Successfully saved"; then
    if [ -f /tmp/sample_surface_test.off ]; then
        echo "PASS: sample_near_surface"
        exit 0
    fi
fi
echo "FAIL: sample_near_surface"
exit 1