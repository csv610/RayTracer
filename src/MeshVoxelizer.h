#ifndef MESH_VOXELIZER_H
#define MESH_VOXELIZER_H

#include "Mesh.h"
#include "RayTracer.h"

/**
 * @class MeshVoxelizer
 * @brief Converts a 3D surface mesh into a volumetric representation.
 * 
 * This class uses ray-tracing to determine the occupancy of a 3D grid.
 * It produces a voxel mesh (a collection of cubes) representing the 
 * internal volume of the input mesh.
 */
class MeshVoxelizer {
public:
    MeshVoxelizer(const Mesh& mesh);
    ~MeshVoxelizer() = default;

    /**
     * @brief Generates a mesh of voxel cubes.
     * @param voxelSize The physical size of each voxel cube.
     */
    Mesh generateVoxelMesh(float voxelSize) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
};

#endif // MESH_VOXELIZER_H
