#ifndef SAMPLE_SURFACE_POINTS_H
#define SAMPLE_SURFACE_POINTS_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

struct SampledPoint {
    Vec3 p;
    Vec3 n;
};

/**
 * @class SampleSurfacePoints
 * @brief Generates high-quality point samples from a mesh surface.
 * 
 * This class provides methods to uniformly sample points and their corresponding 
 * outward-pointing normals from the triangles of a mesh. It uses area-weighted 
 * sampling (via a cumulative distribution function) to ensure unbiased 
 * distribution and employs robust ray-casting to ensure normals are 
 * correctly oriented even for non-manifold geometry.
 * 
 * Results are returned as a list of SampledPoint structures.
 */
class SampleSurfacePoints {
public:
    SampleSurfacePoints(const Mesh& mesh);
    ~SampleSurfacePoints() = default;

    /**
     * @brief Generates surface samples.
     * @param numSamples Target number of samples.
     * @param side Offset samples along the normal: +1 (outside), -1 (inside), 0 (exact surface).
     */
    std::vector<SampledPoint> sample(int numSamples, int side = 0) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    std::vector<float> m_triangleCDF;
    std::vector<int8_t> m_isNormalOutward; 
    float m_totalArea;
    float m_epsilon;

    void buildCDF();
    void precomputeOrientations();
};

#endif // SAMPLE_SURFACE_POINTS_H
