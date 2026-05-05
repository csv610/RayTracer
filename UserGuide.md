# Geometric Analysis and Ray Tracing for Manufacturing

## A Practical Guide to the RayTrace Engine

---

## Chapter 1: Architectural Foundation

The RayTrace engine is built on a modular "Data-Logic-Analysis" pattern:
- **Data (`Mesh.h`)**: Defines `Node`, `Triangle`, and `Mesh` structures.
- **Logic (`MeshGeometry.h`)**: Handles fundamental operations like normals, area, and bounding spheres.
- **Analysis (`src/*.h`)**: Specialized classes (`SkyViewFactor`, `AmbientOcclusion`, `CADFeatureDetector`) that implement high-level manufacturing metrics.

---

## Chapter 2: Core Manufacturing Metrics

### 2.1 Wall Thickness (`StructuralCaliper`)
**Concept:** Measure local material thickness by casting rays inward from the surface.
**Usage:** `./build/structural_caliper_cli mesh.off`

### 2.2 Visibility and Exposure (`SkyViewFactor`)
**Concept:** Fraction of the sky visible from a surface point. 
**Usage:** `./build/sky_view_factor_cli mesh.off -n 128`
- **High SVF (1.0)**: Fully exposed convex surface.
- **Low SVF (0.0)**: Deeply recessed pocket or interior feature.

### 2.3 Feature Detection (`CADFeatureDetector`)
**Concept:** Identifies Pockets, Slots, and Cavities using anisotropic ray probing.
**Usage:** `./build/pocket_detection_cli mesh.off`

---

## Chapter 3: Advanced Analysis

### 3.1 Moldability (`ManufacturingAnalyzer`)
- **Undercuts**: Areas blocked from extraction along a pull direction.
- **Draft Angles**: Surface steepness relative to a mold parting line.

### 3.2 3D Printing (`AutoOrientationOptimizer`)
Analyzes mesh geometry to find the build orientation that minimizes support material or maximizes surface quality.

---

## Chapter 4: Running Tests

The suite includes extensive validation:
- **Unit Tests**: `./build/test_mesh_utils`, `./build/test_geometry_analyzer`
- **Functional Tests**: Managed via CTest.

Run all verifications:
```bash
cd build
ctest --output-on-failure
```

---

## Appendix: Command Reference

| Tool | Purpose | Typical Flags |
|------|---------|---------------|
| `sky_view_factor_cli` | Sky Visibility | `-n 128` (Samples) |
| `pocket_detection_cli` | Feature Analysis | `-n 64` (Samples) |
| `ambient_occlusion_baker_cli` | Surface Shading | `-n 64` (Samples) |
| `mass_properties_cli` | Physical Metrics | `density` |

---
*For undergraduate students: Focus on understanding how ray-surface intersections (`RayTracer::intersect`) enable complex volumetric measurements like mass properties and thickness.*
