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
    
    struct AABB {
        Vec3 min = {1e20f, 1e20f, 1e20f};
        Vec3 max = {-1e20f, -1e20f, -1e20f};
        void expand(const Vertex& v) {
            min.x = std::min(min.x, v.x); min.y = std::min(min.y, v.y); min.z = std::min(min.z, v.z);
            max.x = std::max(max.x, v.x); max.y = std::max(max.y, v.y); max.z = std::max(max.z, v.z);
        }
    } box;

    bool isInside(const Vec3& p) const;
    void buildScene();
};

#endif
