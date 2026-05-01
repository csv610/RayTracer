# Geometric Analysis and Ray Tracing for Manufacturing Applications

## A Practical Guide for Undergraduate Engineering Students

---

## Chapter 1: Introduction

This textbook companion provides hands-on experience with geometric analysis algorithms commonly used in manufacturing, particularly injection molding, CNC machining, and 3D printing. You will learn to use ray tracing techniques to analyze CAD models for manufacturability.

### Prerequisites
- Basic knowledge of 3D geometry (vertices, triangles, meshes)
- Familiarity with coordinate systems and vectors
- Understanding of programming concepts (C++ helpful but not required)

### What is Ray Tracing?

Ray tracing is a technique where we shoot "rays" from a point and trace their paths as they travel through space. When a ray hits a surface, we can compute:
- Whether the surface is visible
- How far away the surface is
- What direction the surface faces (normal)

In manufacturing analysis, we use ray tracing to:
1. Detect undercuts by checking if surfaces are hidden from the pull direction
2. Measure wall thickness by shooting rays inward
3. Calculate visibility and accessibility

---

## Chapter 2: Getting Started

### Building the Software

Open a terminal and navigate to the project directory:

```bash
cd RayTrace
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running Your First Analysis

Let's analyze a simple mesh to understand the workflow:

```bash
./build/mass_properties ../dataset/ter.off 64
```

You should see output like:
```
Mass Properties (density = 1.0):
Volume: 0.194744
CoM:    (-0.030989, -0.009446, 1.409721)
Inertia Tensor at CoM:
|     0.044102    -0.000913    -0.001614 |
...
```

### Understanding the Output Files

Many tools produce `.off` files (Object File Format). These are text-based 3D mesh files that you can open in MeshLab for visualization. Colored faces indicate analysis results:
- **Green** = Good (visible, accessible, thick enough)
- **Red** = Problem area (undercut, inaccessible, too thin)

---

## Chapter 3: Geometric Properties

### 3.1 Volume and Mass Properties

**Concept:** To manufacture a part, we must know its physical properties. Volume calculation using ray integration is more accurate than simple tetrahedron decomposition for complex meshes.

**Algorithm:** The software casts rays through a bounding box and counts how many rays enter and exit the mesh. The difference tells us the volume.

**Try it:**
```bash
./build/mass_properties dataset/ter.off 128
```
Increase resolution for more accuracy.

**Questions to consider:**
- How does changing resolution affect accuracy?
- Where should the center of mass be for a symmetric part?

### 3.2 Symmetry Detection

**Concept:** Symmetric parts are easier to manufacture. Finding symmetry helps determine if a part can be molded in a single cavity.

**Algorithm:**
1. Compute principal axes using Principal Component Analysis (PCA)
2. For each candidate plane, shoot rays to check if vertices reflect correctly
3. Calculate a "symmetry score" - lower is better

**Try it:**
```bash
./build/symmetry_detection dataset/ter.off
```

**Expected output:** The tool finds candidate symmetry planes and scores them. A score < 0.005 indicates strong symmetry.

---

## Chapter 4: Manufacturing Analysis

### 4.1 Undercut Detection

**Concept:** In injection molding, an "undercut" is a feature that prevents the part from being pulled out of the mold. If a surface faces away from the pull direction, it's hidden and causes problems.

**Algorithm:**
1. For each triangle face, compute its normal direction
2. Cast rays from the face in the pull direction
3. If any ray hits another part of the mesh before escaping, it's an undercut

**Key insight:** The pull direction matters! Typically, this is the direction the part moves out of the mold (often Z-axis for vertical presses).

**Try it:**
```bash
./build/undercut_detection dataset/ter.off
```
This uses default pull direction (0,0,1). Try different directions:
```bash
./build/undercut_detection dataset/ter.off undercuts.off 0 1 0
```

**Questions:**
- How many triangles are flagged as undercuts?
- Does changing the pull direction significantly change results?

### 4.2 Draft Angle Analysis

**Concept:** Draft angle is the angle between the mold wall and the part surface. Without sufficient draft, the part will stick to the mold. Typical minimum is 1-3°.

**Algorithm:**
1. For each face, compute the angle between surface normal and pull direction
2. Color-code based on whether angle exceeds threshold

**Try it:**
```bash
./build/draft_angle_analysis dataset/ter.off
```

### 4.3 Wall Thickness Analysis

**Concept:** Parts must be thick enough to be strong, but not so thick that they warp or cost too much material. Wall thickness analysis finds thin regions that might fail.

**Algorithm:**
1. Sample points on the surface
2. Cast rays inward from each point
3. Measure distance to the opposite wall

**Try it:**
```bash
./build/caliper dataset/ter.off
```

**Interpreting results:** Green = thick enough, Red = too thin.

---

## Chapter 5: Additive Manufacturing

### 5.1 Overhang Detection

**Concept:** In 3D printing (especially FDM), material can't be printed on surfaces that overhang too far from below. Angles greater than ~45° from horizontal require support material.

**Algorithm:**
1. For each triangle, compute its angle from horizontal
2. Surfaces facing upward at steep angles need support

**Try it:**
```bash
./build/overhang_analysis dataset/ter.off
```

### 5.2 Optimal Build Orientation

**Concept:** The orientation in which a part is printed affects support material usage, surface finish, and strength. Finding the optimal orientation is an optimization problem.

**Algorithm:**
1. Sample many possible orientations
2. For each, estimate support material needed
3. Select best orientation

**Try it:**
```bash
./build/auto_orientation dataset/ter.off
```

### 5.3 Ray-Traced Slicing

**Concept:** For resin/DLP 3D printing, slices must be computed from the 3D model. Ray tracing gives higher precision than simple intersection.

**Try it:**
```bash
./build/ray_traced_slicer dataset/ter.off 10 128
```
This creates 10 slices at 128×128 resolution.

---

## Chapter 6: CNC Machining Analysis

### 6.1 Accessibility Analysis

**Concept:** A CNC mill can only reach surfaces that are accessible to the tool. Regions hidden from above or blocked by geometry cannot be machined.

**Algorithm:**
1. Define a tool (sphere of given radius)
2. From each surface point, check if tool can approach from above
3. Mark inaccessible regions

**Try it:**
```bash
./build/accessibility_analysis dataset/ter.off
```

### 6.2 Toolpath Simulation

**Concept:** CNC machining removes material. We can simulate this to check for gouging and calculate scallop height (the uncut material between tool paths).

**Try it:**
```bash
./build/cnc_toolpath_sim dataset/ter.off 64 2.0
```
Here 2.0 is the tool radius.

---

## Chapter 7: Surface Analysis

### 7.1 Curvature

**Concept:** Curvature affects how surfaces reflect light and how they feel. Convex regions are easy to polish; concave regions are hard.

**Algorithm:**
- **Gaussian curvature**: product of principal curvatures (K = k₁ × k₂)
- **Mean curvature**: average of principal curvatures (H = (k₁ + k₂)/2)

**Try it:**
```bash
./build/curvature_analysis dataset/ter.off
```

### 7.2 Sky View Factor

**Concept:** The sky view factor is the fraction of visible sky from each point. This affects how quickly paint dries, how much heat accumulates, and more.

**Algorithm:** Cast many rays in a hemisphere and count how many escape without hitting geometry.

**Try it:**
```bash
./build/sky_view_factor dataset/ter.off
```

### 7.3 Pocket Detection

**Concept:** Pockets are deep recessed areas that may be hard to finish or collect debris.

**Try it:**
```bash
./build/pocket_detection dataset/ter.off
```

---

## Chapter 8: Advanced Topics

### 8.1 Signed Distance Fields

**Concept:** A signed distance field (SDF) represents a shape by giving, for every point in space, the distance to the nearest surface. Negative values are inside the mesh.

**Applications:**
- Boolean operations (union, subtraction)
- Smooth blending
- Collision detection

**Try it:**
```bash
./build/sdf_gen dataset/ter.off 32
```

### 8.2 Voxelization

**Concept:** Converting a mesh to a 3D grid of voxels (volume pixels) is useful for simulation and analysis.

**Try it:**
```bash
./build/mesh_voxelizer dataset/ter.off 32
```

### 8.3 Surface Sampling

**Concept:** Sometimes we need points distributed on or near a surface for analysis, simulation, or visualization.

**Outside surface:**
```bash
./build/sample_near_surface dataset/ter.off 1000 1
```

**Inside surface:**
```bash
./build/sample_near_surface dataset/ter.off 1000 -1
```

**Interior sampling:**
```bash
./build/sample_interior dataset/ter.off 1000
```

---

## Chapter 9: Rendering Applications

### 9.1 Basic Ray Tracer

**Concept:** The fundamental operation in ray tracing is finding the closest surface along a ray direction.

**Try it:**
```bash
./build/raytracer dataset/ter.off render.ppm
```
View the PPM file or convert to PNG with ImageMagick:
```bash
convert render.ppm render.png
```

### 9.2 Depth Map

**Concept:** A depth map shows distance from camera to surface. This is fundamental to computer vision and robotics.

**Try it:**
```bash
./build/depth_map dataset/ter.off depth.ppm
```

### 9.3 Shadow Mapping

**Concept:** Secondary rays can determine if a point is in shadow by checking if light can reach it.

**Try it:**
```bash
./build/shadow_plane dataset/ter.off shadow.ppm
```

---

## Chapter 10: Assembly Analysis

### 10.1 Clearance Analysis

**Concept:** When two parts assemble, they must not collide. Clearance analysis checks if parts can fit together with appropriate spacing.

**Requires two mesh files:**
```bash
./build/assembly_clearance partA.off partB.off 1.0
```

### 10.2 Extraction Path Verification

**Concept:** After injection molding, the part must be removed. This analysis checks if there's a collision-free path out of the mold.

**Requires two mesh files:**
```bash
./build/extraction_path_verification part.off mold.off 0 0 1 100
```

---

## Appendix A: File Formats

### OFF Format
The Object File Format (OFF) stores 3D geometry. Basic format:
```
OFF
num_vertices num_faces 0
x y z
x y z
...
3 v0 v1 v2 r g b
3 v0 v1 v2 r g b
...
```

### PPM Format
Portable Pixmap format for images. Binary format that can be viewed in most image software.

### NOFF Format
Extended OFF that includes vertex normals:
```
NOFF
num_vertices num_faces 0
x y z nx ny nz
...
```

---

## Appendix B: Common Issues and Solutions

**Q: My output file is empty or has no colors**
A: Make sure the input mesh is watertight and manifold.

**Q: The analysis takes too long**
A: Reduce sample count or mesh resolution.

**Q: Results don't match expectations**
A: Check that pull direction is correct for your manufacturing process.

---

## Appendix C: Quick Reference

| Tool | Purpose | Basic Command |
|------|---------|---------------|
| mass_properties | Volume, CoM, Inertia | `./mass_properties mesh.off` |
| symmetry_detection | Find symmetry planes | `./symmetry_detection mesh.off` |
| undercut_detection | Find mold undercuts | `./undercut_detection mesh.off` |
| caliper | Wall thickness | `./caliper mesh.off` |
| draft_angle_analysis | Mold draft check | `./draft_angle_analysis mesh.off` |
| overhang_analysis | 3D print support check | `./overhang_analysis mesh.off` |
| accessibility_analysis | CNC reachability | `./accessibility_analysis mesh.off` |
| curvature_analysis | Surface curvature | `./curvature_analysis mesh.off` |
| sky_view_factor | Visibility analysis | `./sky_view_factor mesh.off` |
| pocket_detection | Find cavities | `./pocket_detection mesh.off` |
| mesh_voxelizer | Convert to voxels | `./mesh_voxelizer mesh.off 32` |
| sdf_generator | Create SDF grid | `./sdf_gen mesh.off 32` |
| raytracer | Render mesh | `./raytracer mesh.off output.ppm` |
| depth_map | Depth visualization | `./depth_map mesh.off output.ppm` |

---

*This guide accompanies the RayTrace geometric analysis software suite. For questions or corrections, please refer to the project documentation.*