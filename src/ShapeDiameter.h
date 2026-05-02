#ifndef SHAPE_DIAMETER_H
#define SHAPE_DIAMETER_H

#include <vector>
#include <string>
#include "mesh_utils.h"
#include "RayTracer.h"

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
    const Scene& getScene() const { return scene; }
    void getStats(float& minD, float& maxD, float& avgD) const;

private:
    const Mesh& mesh;
    std::vector<float> shapeDiameters;
    Scene scene;

    void buildScene();
};

#endif // SHAPE_DIAMETER_H
