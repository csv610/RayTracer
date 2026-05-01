#ifndef SHAPE_DIAMETER_H
#define SHAPE_DIAMETER_H

#include <embree4/rtcore.h>
#include <vector>
#include <string>
#include "mesh_utils.h"

class ShapeDiameter {
public:
    struct RayHit {
        int primID;
        Vec3 dir;
        float distance;
    };

    ShapeDiameter(const Mesh& mesh);
    ~ShapeDiameter();

    void compute(int numTheta = 4, int numPhi = 8, float coneAngle = 0.5f);
    void computeForFace(int triIdx, std::vector<RayHit>& hits, int numTheta = 4, int numPhi = 8, float coneAngle = 0.5f) const;

    const std::vector<float>& getDiameters() const { return shapeDiameters; }
    RTCScene getScene() const { return scene; }
    void getStats(float& minD, float& maxD, float& avgD) const;

private:
    const Mesh& mesh;
    std::vector<float> shapeDiameters;
    RTCDevice device = nullptr;
    RTCScene scene = nullptr;

    void buildScene();
};

#endif // SHAPE_DIAMETER_H
