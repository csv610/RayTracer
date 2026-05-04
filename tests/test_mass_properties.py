#!/usr/bin/env python3
import subprocess
import os
import sys

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(project_root)
    input_file = "dataset/ter.off"
    cmd = ["./build/mass_properties", input_file]
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout)
    print(result.stderr, file=sys.stderr)

    if result.returncode != 0:
        print(f"Process failed with return code {result.returncode}")
        sys.exit(1)

    if "Volume:" in result.stdout:
        print("PASS: Success string found")
        sys.exit(0)
    else:
        print("FAIL: Success string not found")
        sys.exit(1)

if __name__ == "__main__":
    main()
