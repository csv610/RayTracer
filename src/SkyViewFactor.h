#ifndef SKY_VIEW_FACTOR_H
#define SKY_VIEW_FACTOR_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class SkyViewFactor
 * @brief Computes the proportion of the sky visible from surface points.
 * 
 * This class uses hemispherical ray casting to determine how much of the
 * celestial hemisphere is unobstructed by the mesh itself. Results are
 * in the range [0, 1], where 1.0 represents full sky visibility and 
 * 0.0 represents complete occlusion.
 */
class SkyViewFactor {
public:
    SkyViewFactor(const Mesh& mesh);
    ~SkyViewFactor() = default;

    /**
     * @brief Computes Sky View Factor for all faces in the mesh.
     * @param samples Number of rays to cast per face.
     * @return Vector of SVF values per face.
     */
    std::vector<float> compute(int samples = 64) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    float m_meshDiag;
};

#endif // SKY_VIEW_FACTOR_H
