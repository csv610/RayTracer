#ifndef MESH_POINT_CLASSIFIER_H
#define MESH_POINT_CLASSIFIER_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class MeshPointClassifier
 * @brief Provides robust point-in-mesh classification.
 * 
 * This class determines if a given point is strictly inside, strictly outside,
 * or on the surface of a closed mesh. It uses a high-reliability approach
 * involving adaptive epsilon selection, multiple-direction parity counting,
 * and localized surface probing to handle degenerate cases and float precision limits.
 * 
 * Primary outputs are integer classifications (-1 for inside, 0 for surface, 1 for outside).
 */
class MeshPointClassifier {
public:
    /**
     * @brief Initialize classifier with a mesh. Builds internal acceleration structures.
     */
    MeshPointClassifier(const Mesh& mesh);

    /**
     * @brief Classify a list of points.
     * @return std::vector<int> containing:
     *         -1 : Point is strictly inside the mesh
     *          0 : Point is on the surface (within epsilon)
     *          1 : Point is strictly outside the mesh
     */
    std::vector<int> classify(const std::vector<Vec3>& points) const;

    /**
     * @brief Individual point classification (efficient for single calls).
     */
    int classify(const Vec3& p) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    AABB m_bbox;
    float m_epsilon;
};

#endif
