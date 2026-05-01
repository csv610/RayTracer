#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing optimal_parting_line..."
output=$(./build/optimal_parting_line dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Optimal Pull Direction:"; then
    echo "PASS: optimal_parting_line"
    exit 0
else
    echo "FAIL: optimal_parting_line"
    exit 1
fi