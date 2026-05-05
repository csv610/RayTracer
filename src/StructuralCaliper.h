#ifndef STRUCTURAL_CALIPER_H
#define STRUCTURAL_CALIPER_H

#include "Mesh.h"
#include "SampleSurfacePoints.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class StructuralCaliper
 * @brief Analyzes a mesh for structural thinness and fragility.
 * 
 * The StructuralCaliper uses surface sampling and ray-casting to measure 
 * the local wall thickness of a model. It identifies regions that fall 
 * below a critical thickness threshold, providing a visual map of 
 * potential structural weak points.
 * 
 * Primary output is an AnalysisResult containing point locations and 
 * their associated thickness values.
 */
class StructuralCaliper {
public:
    struct AnalysisResult {
        Vec3 p;
        float thickness;
        Color4b color;
    };

    StructuralCaliper(const Mesh& mesh);
    ~StructuralCaliper() = default;

    /**
     * @brief Analyzes numSamples points and flags those thinner than threshold.
     */
    std::vector<AnalysisResult> analyze(int numSamples, float threshold) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
};

#endif // STRUCTURAL_CALIPER_H
