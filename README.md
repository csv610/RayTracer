# Embree Ray Tracing Experiments

A high-performance C++ geometric analysis engine for manufacturing and CAD/CAM applications, powered by [Intel® Embree 4](https://www.embree.org/) and **Intel® TBB**.

## Core Architecture

The project features a modernized, modular architecture:
- **Mesh Data Structures**: Clean separation between data (`Mesh.h`) and geometric logic (`MeshGeometry.h`).
- **Standardized Terminology**: Uses `Node`-based indexing consistently across the suite.
- **Ray Tracing Engine**: High-level `Scene` and `RayTracer` abstractions wrapping Embree kernels for safety and readability.
- **Specialized Analyzers**: Independent, focused classes for specific geometric metrics (SVF, AO, Curvature, etc.).

## Project Structure

- `app/`: Command-line applications for manufacturing analysis and rendering.
- `src/`: Core engine, specialized analyzer classes, and unit tests.
- `dataset/`: Sample 3D meshes (OFF format).
- `docs/`: Technical documentation and user guides.

## Dependencies

- **CMake** (version 3.16+)
- **Intel Embree 4**
- **Intel TBB** (Threading Building Blocks)
- **Assimp** (Robust mesh loading)
- **Qt6** (Optional, for visualization)
- **C++17 Compiler**

## Quick Start

### 1. Build the Project
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. Run Tests
```bash
# Run all unit and functional tests
ctest --output-on-failure
```

## Key Applications

### Manufacturing & CAD Analysis
- **sky_view_factor**: Global visibility analysis for exposure and drainage.
- **ambient_occlusion_baker**: Per-face occlusion for realistic shadowing.
- **pocket_detection**: Advanced node-based cavity and slot identification.
- **undercut_detection**: Moldability analysis relative to pull directions.
- **overhang_analysis**: 3D printing support requirement analysis.
- **structural_caliper**: Local wall thickness measurement via ray-casting.
- **accessibility_analysis**: 3-axis CNC tool reachability verification.
- **mass_properties**: High-precision Volume, CoM, and Inertia calculation.
- **symmetry_detection**: Automated principal axis symmetry identification.

### Voxel & Field Generation
- **mesh_voxelizer**: Parity-based volumetric grid generation.
- **sdf_generator**: Signed Distance Field calculation for collision and blending.

## Documentation
See [`UserGuide.md`](UserGuide.md) for a comprehensive guide to geometric analysis algorithms and application usage.

## License
MIT
