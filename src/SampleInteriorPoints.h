#ifndef SAMPLE_INTERIOR_POINTS_H
#define SAMPLE_INTERIOR_POINTS_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class SampleInteriorPoints
 * @brief Generates uniform point samples within the volume of a mesh.
 * 
 * This class uses a rejection sampling algorithm to produce a set of points 
 * that are guaranteed to be inside the mesh boundaries. It is useful for 
 * volumetric analysis, finite element mesh generation, and physical 
 * simulation initialization.
 * 
 * It leverages the Embree-backed Scene for fast point-in-mesh classification.
 */
class SampleInteriorPoints {
public:
    SampleInteriorPoints(const Mesh& mesh);
    ~SampleInteriorPoints() = default;

    /**
     * @brief Generates numSamples points uniformly distributed inside the mesh volume.
     */
    std::vector<Vec3> sample(int numSamples) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    AABB m_bbox;
};

#endif // SAMPLE_INTERIOR_POINTS_H
