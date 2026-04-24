# Embree Ray Tracing Experiments

A collection of C++ applications demonstrating various ray tracing techniques using the [Intel® Embree 4](https://www.embree.org/) ray tracing kernels.

## Project Structure

- `app/`: Source code for the main ray tracing applications.
- `src/`: Shared utility header (`mesh_utils.h`) and unit tests.
- `dataset/`: Sample 3D meshes (OFF format).
- `build/`: Build artifacts and generated renders.

## Dependencies

- **CMake** (version 3.16+)
- **Intel Embree 4**
- **C++17 Compiler**

## Quick Start

### 1. Build the Project
```bash
mkdir build && cd build
cmake ..
make
```

### 2. Run Unit Tests
```bash
./test_mesh_utils
```

### 3. Run a Sample App
```bash
./raytracer ../dataset/ter.off render.ppm
```

## Applications

- **raytracer**: Basic mesh rendering with normal shading.
- **shadow_plane**: Renders a mesh on a ground plane with ray-traced shadows.
- **depth_map**: Visualizes scene depth as a grayscale image.
- **meshvis**: Computes and visualizes face visibility.
- **shape_diameter**: Calculates and visualizes shape diameter function (SDF) values.
- **sphere_benchmark**: Performance benchmark for sphere intersection.

## License
MIT
