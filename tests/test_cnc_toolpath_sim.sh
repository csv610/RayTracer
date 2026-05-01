#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing cnc_toolpath_sim..."
output=$(./build/cnc_toolpath_sim dataset/ter.off 16 2.0 /tmp/cnc_test.off)
echo "$output"

if echo "$output" | grep -q "CNC simulation saved to"; then
    if [ -f /tmp/cnc_test.off ]; then
        echo "PASS: cnc_toolpath_sim"
        exit 0
    fi
fi
echo "FAIL: cnc_toolpath_sim"
exit 1