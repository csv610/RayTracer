#ifndef SAMPLE_SURFACE_H
#define SAMPLE_SURFACE_H

#include "mesh_utils.h"
#include <vector>
#include <embree4/rtcore.h>

struct SampledPoint {
    Vec3 p;
    Vec3 n;
};

class SampleSurface {
public:
    SampleSurface(const Mesh& mesh);
    ~SampleSurface();

    // side = +1 (outside), -1 (inside), 0 (on surface)
    std::vector<SampledPoint> sample(int numSamples, int side = 0) const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    std::vector<float> triangleCDF;
    std::vector<char> isNormalOutward; // char for thread-safety (vector<bool> is not)
    float totalArea;
    float internalEpsilon;

    void buildCDF();
    void buildScene();
    void precomputeOrientations();
    bool checkInside(const Vec3& p) const;
};

#endif
