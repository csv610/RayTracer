# User Guide

This guide provides detailed instructions on how to use the various ray tracing applications in this project.

## 1. Ray Tracer (`raytracer`)
A basic renderer that loads an OFF mesh and outputs a PPM image.
- **Usage:** `./raytracer <input.off> [output.ppm]`
- **Example:** `./raytracer ../dataset/ter.off render.ppm`
- **Output:** A grayscale image where brightness is determined by the dot product of the surface normal and the camera direction.

## 2. Shadow on Plane (`shadow_plane`)
Demonstrates secondary rays by casting shadows from a point light source onto a ground plane.
- **Usage:** `./shadow_plane <input.off> [output.ppm]`
- **Example:** `./shadow_plane ../dataset/ter.off shadow.ppm`
- **Output:** A colored render (Red mesh, Grey plane) with realistic ray-traced shadows.

## 3. Depth Map (`depth_map`)
Visualizes the distance of every pixel to the nearest geometry.
- **Usage:** `./depth_map <input.off> [output.ppm]`
- **Example:** `./depth_map ../dataset/ter.off depth.ppm`
- **Output:** A grayscale map where white is the closest point and black is the farthest point (or background).

## 4. Mesh Visibility (`meshvis`)
Calculates the visibility of each triangle face by sampling rays on a hemisphere.
- **Usage:** `./meshvis <input.off> [output.off]`
- **Example:** `./meshvis ../dataset/ter.off vis_output.off`
- **Output:** An OFF file where face colors indicate visibility (Green = Visible, Red = Occluded).

## 5. Shape Diameter (`shape_diameter`)
Estimates the local thickness of a mesh using ray tracing.
- **Usage:** `./shape_diameter <input.off> [output.off]`
- **Example:** `./shape_diameter ../dataset/ter.off sdf_output.off`
- **Output:** An OFF file color-coded with a Jet colormap based on local thickness.

## 6. Sphere Benchmark (`sphere_benchmark`)
Measures the performance of Embree ray-mesh intersections.
- **Usage:** `./sphere_benchmark [numRays] [stacks] [slices]`
- **Example:** `./sphere_benchmark 1000000 64 128`
- **Output:** Performance metrics including rays per second and total time.

---

### Shared Format: OFF
Most applications use the `.off` (Object File Format) for 3D meshes. You can use tools like [MeshLab](https://www.meshlab.net/) to view the output meshes.

### Shared Format: PPM
Rendered images are saved in `.ppm` (Portable Pixmap) format. These can be opened by most image viewers or converted using tools like ImageMagick.
