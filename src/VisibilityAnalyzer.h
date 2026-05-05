#ifndef VISIBILITY_ANALYZER_H
#define VISIBILITY_ANALYZER_H

#include <vector>
#include "Mesh.h"
#include "RayTracer.h"

/**
 * @class VisibilityAnalyzer
 * @brief Evaluates the line-of-sight visibility of mesh faces.
 * 
 * This class determines if faces are visible from the exterior by sampling 
 * rays over a hemisphere around each face normal. It uses ray tracing to 
 * detect if any sample ray can escape the mesh bounding volume without occlusion.
 */
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
