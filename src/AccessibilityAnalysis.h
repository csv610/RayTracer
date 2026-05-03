#ifndef ACCESSIBILITY_ANALYSIS_H
#define ACCESSIBILITY_ANALYSIS_H

#include <vector>
#include <string>
#include "mesh_utils.h"
#include "RayTracer.h"

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
