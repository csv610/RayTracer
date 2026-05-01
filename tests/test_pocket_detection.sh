#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing pocket_detection..."
output=$(./build/pocket_detection dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Pocket analysis complete"; then
    echo "PASS: pocket_detection"
    exit 0
else
    echo "FAIL: pocket_detection"
    exit 1
fi