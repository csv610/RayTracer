#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing projected_thickness..."
output=$(./build/projected_thickness dataset/ter.off 16)
echo "$output"

if echo "$output" | grep -q "Thickness analysis saved"; then
    echo "PASS: projected_thickness"
    exit 0
else
    echo "FAIL: projected_thickness"
    exit 1
fi