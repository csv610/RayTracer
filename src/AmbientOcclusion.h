#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class AmbientOcclusion
 * @brief Computes ambient occlusion for surface nodes.
 * 
 * Ambient Occlusion (AO) measures how much a point on a surface is 
 * exposed to ambient lighting. Points in deep crevices or pockets 
 * will have higher AO values (darker), while exposed points will 
 * have lower values. Results are in the range [0, 1].
 */
class AmbientOcclusion {
public:
    AmbientOcclusion(const Mesh& mesh);
    ~AmbientOcclusion() = default;

    /**
     * @brief Computes Ambient Occlusion for all nodes in the mesh.
     * @param samples Number of rays to cast per node.
     * @return Vector of AO values per node.
     */
    std::vector<float> compute(int samples = 64) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    float m_meshDiag;
};

#endif // AMBIENT_OCCLUSION_H
