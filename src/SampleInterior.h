#ifndef SAMPLE_INTERIOR_H
#define SAMPLE_INTERIOR_H

#include "mesh_utils.h"
#include <vector>
#include <embree4/rtcore.h>

class SampleInterior {
public:
    SampleInterior(const Mesh& mesh);
    ~SampleInterior();

    // Generates numSamples points uniformly distributed inside the mesh volume
    std::vector<Vec3> sample(int numSamples) const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    AABB box;

    bool isInside(const Vec3& p) const;
    void buildScene();
};

#endif
