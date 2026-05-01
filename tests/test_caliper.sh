#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing caliper..."
output=$(./build/caliper dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Successfully saved"; then
    echo "PASS: caliper"
    exit 0
else
    echo "FAIL: caliper"
    exit 1
fi