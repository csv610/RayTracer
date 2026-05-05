#ifndef ACCESSIBILITY_ANALYSIS_H
#define ACCESSIBILITY_ANALYSIS_H

#include <vector>
#include <string>
#include "Mesh.h"
#include "RayTracer.h"

/**
 * @class AccessibilityAnalysis
 * @brief Determines the geometric accessibility of mesh faces from a vertical direction (+Z axis).
 * 
 * This class uses ray-tracing to simulate a cylindrical tool of a given radius.
 * It identifies faces as inaccessible if they are downward-facing or occluded
 * by other geometry when approached from above.
 * 
 * Results are stored as face colors (Green for accessible, Red for inaccessible).
 */
class AccessibilityAnalysis {
public:
    AccessibilityAnalysis(const Mesh& mesh);
    ~AccessibilityAnalysis();

    void analyze(float toolRadius);

    const std::vector<Color4b>& getTriColors() const { return triColors_; }
    int getInaccessibleCount() const { return inaccessibleCount_; }

private:
    const Mesh& mesh_;
    Scene scene_;
    std::vector<Color4b> triColors_;
    int inaccessibleCount_ = 0;
};

#endif // ACCESSIBILITY_ANALYSIS_H
