#ifndef MANUFACTURING_ANALYZER_H
#define MANUFACTURING_ANALYZER_H

#include "mesh_utils.h"
#include <vector>
#include <embree4/rtcore.h>

class ManufacturingAnalyzer {
public:
    struct Result {
        std::vector<Vec3> colors;
        int count = 0;
        Mesh getColoredMesh(const Mesh& original) const {
            Mesh m = original;
            m.faceColors = colors;
            return m;
        }
    };

    ManufacturingAnalyzer(const Mesh& mesh);
    ~ManufacturingAnalyzer();

    // Pull/Up direction analysis
    Result analyzeUndercuts(Vec3 pullDir) const;
    Result analyzeOverhangs(float thresholdDeg) const;
    Result analyzeDraftAngles(Vec3 pullDir) const;

    // Optimization
    Vec3 findOptimalPartingLine(int numSamples = 64) const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    float meshDiag;

    void buildScene();
    float calculateUndercutScore(Vec3 pullDir) const;
};

#endif
