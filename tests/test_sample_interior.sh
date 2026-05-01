#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing sample_interior..."
output=$(./build/sample_interior dataset/ter.off 100 /tmp/sample_interior_test.off)
echo "$output"

if echo "$output" | grep -q "Successfully saved"; then
    if [ -f /tmp/sample_interior_test.off ]; then
        echo "PASS: sample_interior"
        exit 0
    fi
fi
echo "FAIL: sample_interior"
exit 1