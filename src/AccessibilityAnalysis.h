#ifndef ACCESSIBILITY_ANALYSIS_H
#define ACCESSIBILITY_ANALYSIS_H

#include <embree4/rtcore.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <tbb/parallel_for.h>
#include "mesh_utils.h"

class AccessibilityAnalysis {
public:
    AccessibilityAnalysis(const Mesh& mesh);
    ~AccessibilityAnalysis();

    void analyze(float toolRadius);

    const std::vector<Vec3>& getTriColors() const { return triColors_; }
    int getInaccessibleCount() const { return inaccessibleCount_; }

private:
    const Mesh& mesh_;
    RTCDevice device_ = nullptr;
    RTCScene scene_ = nullptr;
    std::vector<Vec3> triColors_;
    int inaccessibleCount_ = 0;
};

#endif // ACCESSIBILITY_ANALYSIS_H
