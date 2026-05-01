# Embree Ray Tracing Experiments

A collection of C++ applications demonstrating various ray tracing techniques and geometric analysis tools using the [Intel® Embree 4](https://www.embree.org/) ray tracing kernels and **Intel® TBB** for parallelization.

## Project Structure

- `app/`: Source code for the main ray tracing and analysis applications.
- `src/`: Core logic, shared utility headers, and unit tests.
- `tests/`: Shell script tests and C++ unit tests for all apps.
- `dataset/`: Sample 3D meshes (OFF format).
- `build/`: Build artifacts and generated renders.

## Dependencies

- **CMake** (version 3.16+)
- **Intel Embree 4**
- **Intel TBB** (Threading Building Blocks)
- **Assimp** (for robust mesh loading)
- **Qt6** (optional, for visualization tools)
- **C++20 Compiler**

## Quick Start

### 1. Build the Project
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. Run Tests
```bash
# Run C++ unit tests
cd build
ctest --output-on-failure

# Or run shell script tests manually
cd ..
./tests/test_mass_properties.sh
./tests/test_symmetry_detection.sh
# ... (see tests/ directory for all tests)
```

## Testing

This project includes comprehensive tests for all applications:

### C++ Unit Tests (`test_apps`)
- `test_mesh_utils` - Mesh utility functions
- `test_mesh_io` - File I/O operations
- `test_aabb` - Bounding box operations
- `test_jet_color` - Color mapping
- `test_physical_properties` - Volume, center of mass, inertia tensor
- `test_symmetry_detector` - Symmetry plane detection
- `test_structural_caliper` - Wall thickness analysis
- `test_manufacturing_analyzer` - Draft angles, undercuts, overhangs
- `test_assembly_analyzer` - Clearance, extraction path
- `test_geometry_analyzer` - Ambient occlusion, sky view factor

### Shell Script Tests (`tests/*.sh`)
All 23 single-mesh applications have automated tests that verify correct execution and output generation.

Run all tests:
```bash
cd build
ctest --output-on-failure
```

## Applications

### Geometric Analysis Tools (CAD/CAM Focused)
- **symmetry_detection**: Identifies candidate symmetry planes using PCA and evaluates them via ray-traced reflection error.
- **pocket_detection**: Detects deep cavities and recessed features using hemispherical exposure scoring.
- **undercut_detection**: Identifies structural and occluded undercuts relative to a specified pull direction for injection molding.
- **overhang_analysis**: Flags surfaces requiring support material in 3D printing based on a critical angle threshold.
- **draft_angle_analysis**: Calculates and visualizes draft angles for moldability analysis.
- **mass_properties**: Calculates Volume, Center of Mass, and Inertia Tensor using high-density ray integration.
- **auto_orientation**: Finds the optimal build orientation to minimize support material for 3D printing.
- **ray_traced_slicer**: Generates high-resolution 2D slice bitmaps for resin/DLP 3D printing.
- **cnc_toolpath_sim**: Simulates material removal and scallop height for CNC milling verification.
- **inter_part_visibility**: Analyzes line-of-sight visibility of a part from a viewer's perspective within an assembly.
- **sky_view_factor**: Computes the Sky View Factor (global visibility) for every face.
- **extraction_path_verification**: Checks if a part can be removed from an environment along a translation vector without collisions.
- **optimal_parting_line**: Automatically finds the best pull direction to minimize undercuts for mold design.
- **accessibility_analysis**: Verifies if mesh regions are reachable by a 3-axis CNC tool of a given radius.
- **assembly_clearance**: Detects collisions and clearance violations between two distinct mesh parts.
- **structural_caliper**: Analyzes local mesh thickness using inward ray casting to identify thin wall regions.
- **projected_thickness**: Computes vertex-based projected thickness along surface normals.
- **curvature_analysis**: Computes Gaussian and mean curvature, along with concavity scores.
- **mesh_voxelizer**: Converts a triangular mesh into a voxel grid representation using parity-based interior testing.
- **sdf_generator**: Generates a signed distance field grid around the mesh.
- **mesh_visibility**: Computes per-face visibility using hemispherical ray sampling.
- **sample_interior**: Samples random points inside the mesh volume.
- **sample_near_surface**: Samples points near the mesh surface (outside, inside, or on surface).

### Rendering & Visualization
- **raytracer**: Basic mesh rendering with normal-based shading.
- **ambient_occlusion_baker**: Computes per-face ambient occlusion using hemispherical sampling.
- **shadow_plane**: Renders a mesh on a ground plane with ray-traced shadows.
- **depth_map**: Visualizes scene depth as a grayscale image.
- **meshvis**: Computes and visualizes face visibility from a bounding sphere.
- **shape_diameter**: Calculates and visualizes the Shape Diameter Function (SDF).
- **depth_map_viewer**: Qt-based interactive depth map visualization.
- **shape_diameter_vis**: Interactive visualization for SDF analysis.

### Benchmarking
- **sphere_benchmark**: Performance benchmark for ray-sphere intersection using generated UV spheres.

## License
MIT
