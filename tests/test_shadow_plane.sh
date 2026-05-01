#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing shadow_plane..."
output=$(./build/shadow_plane dataset/ter.off /tmp/shadow_test.ppm)
echo "$output"

if echo "$output" | grep -q "Saved to"; then
    if [ -f /tmp/shadow_test.ppm ]; then
        echo "PASS: shadow_plane"
        exit 0
    fi
fi
echo "FAIL: shadow_plane"
exit 1