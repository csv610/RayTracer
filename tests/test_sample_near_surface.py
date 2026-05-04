#!/usr/bin/env python3
import subprocess
import os
import sys

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(project_root)
    input_file = "dataset/ter.off"
    output_file = "/tmp/test_test_sample_near_surface.off"
    if os.path.exists(output_file): os.remove(output_file)
    cmd = ["./build/sample_near_surface", input_file, "-o", output_file]
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout)
    print(result.stderr, file=sys.stderr)

    if result.returncode != 0:
        print(f"Process failed with return code {result.returncode}")
        sys.exit(1)

    if os.path.exists("/tmp/test_test_sample_near_surface.off"):
        print("PASS: Output file exists")
        sys.exit(0)
    else:
        print("FAIL: Output file does not exist")
        sys.exit(1)

if __name__ == "__main__":
    main()
