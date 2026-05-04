#!/bin/bash
set -e
cd "$(dirname "$0")/.."
echo "Testing inter_part_visibility..."
OUTPUT="/tmp/inter_vis.off"
./build/inter_part_visibility dataset/ter.off dataset/ter.off -d "0 0 1" -o $OUTPUT

if [ -f "$OUTPUT" ]; then
    echo "PASS: inter_part_visibility"
    exit 0
fi
echo "FAIL: inter_part_visibility"
exit 1
