#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing mass_properties..."
output=$(./build/mass_properties dataset/ter.off 64)
echo "$output"

if echo "$output" | grep -q "Volume:"; then
    echo "PASS: mass_properties"
    exit 0
else
    echo "FAIL: mass_properties"
    exit 1
fi