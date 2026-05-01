#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Testing mesh_voxelizer..."
output=$(./build/mesh_voxelizer dataset/ter.off 16)
echo "$output"

if echo "$output" | grep -q "Voxelization saved to"; then
    echo "PASS: mesh_voxelizer"
    exit 0
else
    echo "FAIL: mesh_voxelizer"
    exit 1
fi