#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing auto_orientation..."
output=$(./build/auto_orientation dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Rank"; then
    echo "PASS: auto_orientation"
    exit 0
else
    echo "FAIL: auto_orientation"
    exit 1
fi