#ifndef SYMMETRY_DETECTOR_H
#define SYMMETRY_DETECTOR_H

#include "mesh_utils.h"
#include "SampleSurface.h"
#include <vector>
#include <embree4/rtcore.h>

struct Plane {
    Vec3 normal;
    float distance; // n.p = d

    Vec3 reflectPoint(const Vec3& p) const {
        float d = (normal.x * p.x + normal.y * p.y + normal.z * p.z) - distance;
        return {p.x - 2 * d * normal.x, p.y - 2 * d * normal.y, p.z - 2 * d * normal.z};
    }

    Vec3 reflectVector(const Vec3& v) const {
        float d = (normal.x * v.x + normal.y * v.y + normal.z * v.z);
        return {v.x - 2 * d * normal.x, v.y - 2 * d * normal.y, v.z - 2 * d * normal.z};
    }
};

class SymmetryDetector {
public:
    SymmetryDetector(const Mesh& mesh);
    ~SymmetryDetector();

    // Returns a symmetry score (lower is better, 0 is perfect symmetry)
    float checkSymmetry(const Plane& plane) const;

    // Finds candidate planes based on principal axes
    std::vector<Plane> findCandidatePlanes() const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    float meshDiagonal;
    std::vector<SampledPoint> preSampledPoints;

    void buildScene();
    void preSample(int numSamples);
};

#endif
