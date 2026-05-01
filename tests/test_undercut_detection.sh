#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing undercut_detection..."
output=$(./build/undercut_detection dataset/ter.off)
echo "$output"

if echo "$output" | grep -q "Detected.*undercut triangles"; then
    echo "PASS: undercut_detection"
    exit 0
else
    echo "FAIL: undercut_detection"
    exit 1
fi