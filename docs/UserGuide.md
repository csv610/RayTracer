# User Guide: Embree Ray Tracing & Geometric Analysis Toolkit

This guide provides detailed instructions on how to use the various tools in the RayTrace project for CAD/CAM analysis, mesh processing, and rendering.

---

## 1. Manufacturing & CAD Analysis

### CNC Accessibility Analysis (`accessibility_analysis`)
Verifies if a 3-axis CNC milling tool can reach specific regions of a mesh without the tool shaft colliding with the part.
- **Usage:** `./accessibility_analysis <mesh.off> [tool_radius] [output.off]`
- **Visualization:** 
  - **Green:** Accessible regions.
  - **Red:** Inaccessible regions (due to verticality or tool-shaft collision).

### Assembly Clearance & Collision (`assembly_clearance`)
Detects hard collisions and clearance tolerance violations between two parts.
- **Usage:** `./assembly_clearance <partA.off> <partB.off> [clearance_threshold] [outputA_colored.off]`
- **Visualization:**
  - **Red:** Hard collision (Part A is inside Part B).
  - **Orange:** Clearance violation (Distance < threshold).
  - **Green:** Safe regions.

### Undercut Detection (`undercut_detection`)
Identifies areas that cannot be ejected from a two-part mold along a specific pull direction.
- **Usage:** `./undercut_detection <mesh.off> [output.off] [pull_x pull_y pull_z]`
- **Visualization:**
  - **Red:** Undercut (occluded or facing away from pull).
  - **Green:** Safe to eject.

### Draft Angle Analysis (`draft_angle_analysis`)
Calculates the angle between the face normal and the pull direction to ensure moldability.
- **Usage:** `./draft_angle_analysis <mesh.off> [pull_x pull_y pull_z] [output.off]`
- **Color Scale:** Heatmap from Red (undercut/low draft) to Green (safe draft angle > 3°).

### Pocket Detection (`pocket_detection`)
Identifies deep, concave regions or "pockets" using hemispherical ray casting.
- **Usage:** `./pocket_detection <mesh.off> [output.off] [samples_per_face]`
- **Visualization:** Red (deep pockets) to White (fully exposed surfaces).

---

## 2. Geometric Analysis

### Symmetry Detection (`symmetry_detection`)
Finds the best symmetry plane for a mesh among its principal axes.
- **Usage:** `./symmetry_detection <mesh.off>`
- **Output:** Prints symmetry scores for candidate planes and flags strong/moderate symmetry.

### Structural Thickness (`structural_caliper` & `projected_thickness`)
Analyzes local thickness to identify thin walls.
- **Usage (Caliper):** `./caliper <mesh.off> [threshold] [samples] [output.off]`
- **Usage (Projected):** `./projected_thickness <mesh.off> [samples] [output.off]`
- **Visualization:** Red (thin regions) to Green/Blue (thick regions).

### Curvature Analysis (`curvature_analysis`)
Computes Gaussian and Mean curvature to detect features.
- **Usage:** `./curvature_analysis <mesh.off> [output.off]`

---

## 3. Mesh Processing & Rendering

### Mesh Voxelizer (`mesh_voxelizer`)
Converts a triangular mesh into a voxel grid.
- **Usage:** `./mesh_voxelizer <mesh.off> [resolution] [output_voxels.off]`
- **Output:** A mesh composed of cubes representing the interior volume.

### Ambient Occlusion Baker (`ao_baker`)
Bakes soft shadows into the mesh faces.
- **Usage:** `./ao_baker <mesh.off> [output.off] [samples]`
- **Visualization:** Grayscale mesh where darker areas represent occluded regions.

### Shape Diameter Function (`shape_diameter`)
Calculates local volume thickness.
- **Usage:** `./shape_diameter <mesh.off> [output.off]`

---

## 4. Basic Rendering

### Standard Raytracer (`raytracer`)
Simple normal-based rendering to a PPM image.
- **Usage:** `./raytracer <mesh.off> [output.ppm]`

### Shadow Plane (`shadow_plane`)
Renders the mesh on a ground plane with shadows.
- **Usage:** `./shadow_plane <mesh.off> [output.ppm]`

---

## 5. Tips for Visualization

Most analysis tools output a **colored OFF file**. These can be viewed using standard mesh viewers like **MeshLab** or **CloudCompare**.

- **To view colors in MeshLab:**
  1. Open the `.off` file.
  2. In the "Layer Properties" or "Render" menu, ensure "Color" is set to "Per-Face" or "Per-Vertex" depending on the tool's output.
