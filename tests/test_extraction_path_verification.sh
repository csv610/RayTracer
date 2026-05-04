#!/bin/bash
set -e
cd "$(dirname "$0")/.."
echo "Testing extraction_path_verification..."
# Note: extraction_path_verification_cli.cpp expects positional 'part' and 'environment'
output=$(./build/extraction_path_verification dataset/ter.off dataset/ter.off -o /tmp/extraction.off -d "0 1 0" -l 10)
echo "$output"
if [ -f /tmp/extraction.off ]; then
    echo "PASS: extraction_path_verification"
    exit 0
fi
echo "FAIL: extraction_path_verification"
exit 1
