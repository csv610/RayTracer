#!/bin/bash
cd "$(dirname "$0")/.."
INPUT="dataset/ter.off"
OUTPUT="/tmp/test_$(basename $0 .sh).off"

RESULT=$(./build/mass_properties $INPUT -o $OUTPUT)
echo "$RESULT"

if echo "$RESULT" | grep -q "Volume:"; then
  exit 0
else
  exit 1
fi
