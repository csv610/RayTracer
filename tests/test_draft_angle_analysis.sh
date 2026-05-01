#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing draft_angle_analysis..."
output=$(./build/draft_angle_analysis dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Draft angle analysis saved"; then
    echo "PASS: draft_angle_analysis"
    exit 0
else
    echo "FAIL: draft_angle_analysis"
    exit 1
fi