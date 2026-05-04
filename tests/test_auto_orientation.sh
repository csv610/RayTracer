#!/bin/bash
cd "$(dirname "$0")/.."
INPUT="dataset/ter.off"
OUTPUT="/tmp/test_$(basename $0 .sh).off"

# Run the app and capture output
RESULT=$(./build/auto_orientation $INPUT -o $OUTPUT)
echo "$RESULT"

# Check for success string
if echo "$RESULT" | grep -q "Optimal Build Orientation Analysis:"; then
  exit 0
else
  exit 1
fi
