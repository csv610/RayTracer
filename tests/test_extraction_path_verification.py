#!/usr/bin/env python3
import subprocess
import os
import sys

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(project_root)
    input_file = "dataset/ter.off"
    output_file = "/tmp/extraction.off"
    if os.path.exists(output_file): os.remove(output_file)
    cmd = ["./build/extraction_path_verification", input_file, input_file, "-o", output_file, "-d", "0 1 0", "-l", "10"]
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout)
    print(result.stderr, file=sys.stderr)

    if result.returncode != 0:
        print(f"Process failed with return code {result.returncode}")
        sys.exit(1)

    if os.path.exists(output_file):
        print("PASS: Output file exists")
        sys.exit(0)
    else:
        print("FAIL: Output file does not exist")
        sys.exit(1)

if __name__ == "__main__":
    main()
