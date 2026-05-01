#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing mesh_visibility..."
output=$(./build/mesh_visibility dataset/ter.off /tmp/mesh_visibility_test.off)
echo "$output"

if echo "$output" | grep -q "Ray hits:"; then
    if [ -f /tmp/mesh_visibility_test.off ]; then
        echo "PASS: mesh_visibility"
        exit 0
    fi
fi
echo "FAIL: mesh_visibility"
exit 1