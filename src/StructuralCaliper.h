#ifndef STRUCTURAL_CALIPER_H
#define STRUCTURAL_CALIPER_H

#include "mesh_utils.h"
#include "SampleSurface.h"
#include <vector>
#include <embree4/rtcore.h>

class StructuralCaliper {
public:
    struct AnalysisResult {
        Vec3 p;
        float thickness;
        Vec3 color; // Red-to-Green based on threshold
    };

    StructuralCaliper(const Mesh& mesh);
    ~StructuralCaliper();

    // Analyzes numSamples points and flags those thinner than threshold
    std::vector<AnalysisResult> analyze(int numSamples, float threshold) const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;

    void buildScene();
};

#endif
