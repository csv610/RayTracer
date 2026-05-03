#ifndef INSIDE_OUTSIDE_KERNEL_H
#define INSIDE_OUTSIDE_KERNEL_H

#include "mesh_utils.h"
#include "RayTracer.h"
#include <vector>

class InsideOutsideKernel {
public:
    /**
     * @brief Initialize kernel with a mesh. Builds internal acceleration structures.
     */
    InsideOutsideKernel(const Mesh& mesh);

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
