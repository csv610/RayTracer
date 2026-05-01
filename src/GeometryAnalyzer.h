#ifndef GEOMETRY_ANALYZER_H
#define GEOMETRY_ANALYZER_H

#include "mesh_utils.h"
#include <embree4/rtcore.h>
#include <vector>

class GeometryAnalyzer {
public:
    GeometryAnalyzer(const Mesh& mesh);
    ~GeometryAnalyzer();

    // Visibility-based tools
    std::vector<float> computeAmbientOcclusion(int samples = 64) const;
    std::vector<float> computeSkyViewFactor(int samples = 64) const;
    std::vector<float> computePocketExposure(int samples = 64) const;

    // Curvature
    struct CurvatureResult {
        std::vector<float> gaussian;
        std::vector<float> mean;
    };
    CurvatureResult analyzeCurvature() const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    float meshDiag;

    void buildScene();
    std::vector<float> runHemisphericalSampling(int samples, bool invert) const;
};

#endif
