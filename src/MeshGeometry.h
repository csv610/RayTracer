#ifndef MESH_GEOMETRY_H
#define MESH_GEOMETRY_H

#include "Mesh.h"
#include "MeshTopology.h"
#include <vector>

/**
 * @class MeshGeometry
 * @brief Provides intrinsic geometric metric calculations.
 * 
 * Focuses on properties derived from node positions and face areas.
 * It utilizes MeshTopology for neighborhood-based calculations like Curvature.
 */
class MeshGeometry {
public:
    MeshGeometry(const Mesh& mesh);

    // Static Primitives (formerly in Mesh.h)
    static Vec3 computeFaceNormal(const Node& v0, const Node& v1, const Node& v2);
    static float computeFaceArea(const Node& v0, const Node& v1, const Node& v2);
    static Vec3 computeFaceCenter(const Node& v0, const Node& v1, const Node& v2);

    // Normal calculations
    Vec3 computeFaceNormal(unsigned int triIdx) const;
    std::vector<Vec3> computeAllFaceNormals() const;
    std::vector<Vec3> computeNodeNormals() const;

    // Face Centers (Requested)
    Vec3 computeFaceCenter(unsigned int triIdx) const;
    std::vector<Vec3> computeAllFaceCenters() const;

    // Curvature (Utilizes MeshTopology internally)
    struct Curvature {
        std::vector<float> gaussian;
        std::vector<float> mean;
    };
    Curvature computeCurvature() const;

    // Utilities
    float computeFaceArea(unsigned int triIdx) const;
    std::vector<float> computeAllFaceAreas() const;
    std::vector<float> computeNodeAreas() const;

    // Bounding Volumes
    AABB computeAABB() const;
    void computeBoundingSphere(Node& center, float& radius) const;

    // Generators and Utilities (Static)
    static void createUVSphere(Mesh& mesh, int stacks, int slices, float radius);
    static Color4b getJetColor(float t);

private:
    const Mesh& m_mesh;
};

#endif // MESH_GEOMETRY_H
