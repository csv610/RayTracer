#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing sdf_generator..."
output=$(./build/sdf_gen dataset/ter.off 16)
echo "$output"

if echo "$output" | grep -q "SDF grid saved to"; then
    echo "PASS: sdf_generator"
    exit 0
else
    echo "FAIL: sdf_generator"
    exit 1
fi