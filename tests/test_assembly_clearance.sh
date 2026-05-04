#!/bin/bash
set -e
cd "$(dirname "$0")/.."
echo "Testing assembly_clearance..."
output=$(./build/assembly_clearance dataset/ter.off dataset/ter.off -o /tmp/clearance.off -t 0.5)
echo "$output"
if [ -f /tmp/clearance.off ]; then
    echo "PASS: assembly_clearance"
    exit 0
fi
echo "FAIL: assembly_clearance"
exit 1
