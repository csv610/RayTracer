#ifndef GEOMETRY_ANALYZER_H
#define GEOMETRY_ANALYZER_H

#include "Mesh.h"
#include "MeshGeometry.h"
#include <embree4/rtcore.h>
#include <vector>

/**
 * @class GeometryAnalyzer
 * @brief High-performance geometric analysis using the Embree ray-tracing engine.
 * 
 * This class provides advanced visibility-based metrics such as Ambient Occlusion,
 * Sky View Factor, and Pocket Exposure. It utilizes low-discrepancy hemispherical
 * sampling to achieve high accuracy and performance.
 * 
 * It is designed for large meshes where optimized spatial acceleration
 * structures are critical for analysis speed.
 */
class GeometryAnalyzer {
public:
    GeometryAnalyzer(const Mesh& mesh);
    ~GeometryAnalyzer();

    // Visibility-based tools
    std::vector<float> computeAmbientOcclusion(int samples = 64) const;
    std::vector<float> computeSkyViewFactor(int samples = 64) const;
    std::vector<float> computePocketExposure(int samples = 64) const;

    // Curvature (delegates to MeshGeometry)
    MeshGeometry::Curvature analyzeCurvature() const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    float meshDiag;

    void buildScene();
    std::vector<float> runHemisphericalSampling(int samples, bool invert) const;
};

#endif
