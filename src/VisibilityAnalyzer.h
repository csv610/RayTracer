#ifndef VISIBILITY_ANALYZER_H
#define VISIBILITY_ANALYZER_H

#include <vector>
#include "mesh_utils.h"
#include "RayTracer.h"

class VisibilityAnalyzer {
public:
    struct Result {
        std::vector<Color4b> colors;
        int visibleCount;
        Mesh getColoredMesh(const Mesh& original) const;
    };

    VisibilityAnalyzer(const Mesh& mesh);
    ~VisibilityAnalyzer();

    Result computeVisibility(int numTheta = 8, int numPhi = 16) const;

private:
    const Mesh& mesh;
    Scene scene;
    float sphereRadius;
    void buildScene();
};

#endif // VISIBILITY_ANALYZER_H
