#!/bin/bash
cd "$(dirname "$0")/.."
INPUT="dataset/ter.off"
OUTPUT="/tmp/test_$(basename $0 .sh).off"

./build/curvature_analysis $INPUT  -o $OUTPUT

if [ -f "$OUTPUT" ]; then
  exit 0
else
  exit 1
fi
