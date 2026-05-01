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

### Extraction Path Verification (`extraction_path_verification`)
Checks if a part can be physically removed from an assembly or environment along a linear translation path.
- **Usage:** `./extraction_path_verification <part.off> <environment.off> [dir_x dir_y dir_z] [distance] [output.off]`
- **Visualization:**
  - **Red:** Triangles that will collide with the environment during extraction.
  - **Green:** Triangles with a clear path.

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

### Auto-Orientation (`auto_orientation`)
Finds the rotation that minimizes the volume of support material needed for 3D printing.
- **Usage:** `./auto_orientation <mesh.off> [num_samples]`
- **Output:** Lists the top orientations based on estimated support volume.

### Ray-Traced Slicer (`ray_traced_slicer`)
Generates high-resolution slice images for resin 3D printers.
- **Usage:** `./ray_traced_slicer <mesh.off> <num_layers> [resolution]`
- **Output:** Saves periodic layer bitmaps as `.ppm` files.

### CNC Toolpath Simulation (`cnc_toolpath_sim`)
Simulates a ball-end milling tool scraping the mesh to visualize the final surface quality.
- **Usage:** `./cnc_toolpath_sim <mesh.off> [resolution] [tool_radius] [output.off]`
- **Visualization:** Blue (perfect match) to Red (high scallop/leftover material).

### Inter-Part Visibility (`inter_part_visibility`)
Analyzes if a specific part is visible to a technician/viewer from a given point in an assembly.
- **Usage:** `./inter_part_visibility <target.off> <environment.off> <viewer_x viewer_y viewer_z> [output.off]`
- **Visualization:** Green (Visible) to Red (Occluded).

### Optimal Parting Line (`optimal_parting_line`)
Automatically finds the pull direction that minimizes the number of undercuts in a part.
- **Usage:** `./optimal_parting_line <mesh.off> [num_search_directions]`
- **Output:** Lists the top 5 pull directions with their respective undercut percentages.

### Mass Properties (`mass_properties`)
Calculates the physical properties of the mesh (Volume, CoM, Inertia).
- **Usage:** `./mass_properties <mesh.off> [resolution]`
- **Note:** Requires a closed, manifold mesh for accurate volume calculation. Resolution defaults to 128.
- **Output:** Prints the Volume, Center of Mass coordinates, and the 3x3 Inertia Tensor at the CoM.

### Sky View Factor (`sky_view_factor`)
Calculates the percentage of the sky visible from each face.
- **Usage:** `./sky_view_factor <mesh.off> [samples] [output.off]`
- **Visualization:** Blue (low visibility) to Red (high visibility).

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
