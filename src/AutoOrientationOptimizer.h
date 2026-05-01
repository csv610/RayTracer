#ifndef AUTO_ORIENTATION_OPTIMIZER_H
#define AUTO_ORIENTATION_OPTIMIZER_H

#include "mesh_utils.h"
#include <embree4/rtcore.h>
#include <vector>

class AutoOrientationOptimizer {
public:
    struct Result {
        Vec3 up;
        float supportVolume;
    };

    AutoOrientationOptimizer(const Mesh& mesh);
    ~AutoOrientationOptimizer();

    std::vector<Result> optimize(int numSamples = 32) const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    float meshDiag;

    void buildScene();
    float calculateSupportVolume(Vec3 upDir) const;
};

#endif
