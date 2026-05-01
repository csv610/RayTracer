#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing overhang_analysis..."
output=$(./build/overhang_analysis dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "overhang triangles"; then
    echo "PASS: overhang_analysis"
    exit 0
else
    echo "FAIL: overhang_analysis"
    exit 1
fi