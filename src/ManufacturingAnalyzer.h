#ifndef MANUFACTURING_ANALYZER_H
#define MANUFACTURING_ANALYZER_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class ManufacturingAnalyzer
 * @brief Evaluates mesh geometry for manufacturing feasibility and constraints.
 * 
 * This class identifies features critical for processes like injection molding,
 * casting, and 3D printing. It detects undercuts (occlusions along a pull direction),
 * overhangs (steep downward faces), and insufficient draft angles.
 * 
 * It can also suggest an optimal parting line by sampling various orientations
 * to find the direction with minimal undercuts.
 */
class ManufacturingAnalyzer {
public:
    struct Result {
        std::vector<Color4b> colors;
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
    Scene scene;
    float meshDiag;

    void buildScene();
    float calculateUndercutScore(Vec3 pullDir) const;
};

#endif
