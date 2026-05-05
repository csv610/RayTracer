#ifndef THICKNESS_ANALYZER_H
#define THICKNESS_ANALYZER_H

#include <vector>
#include "Mesh.h"
#include "RayTracer.h"

/**
 * @class ThicknessAnalyzer
 * @brief Analyzes the wall thickness of a mesh using ray-casting.
 * 
 * This class computes the thickness of a mesh by casting rays from each node
 * into the interior along the inverted node normal. It identifies the distance
 * to the opposite wall to determine local thickness.
 */
class ThicknessAnalyzer {
public:
    struct Result {
        std::vector<float> thickness;
        float maxThickness;
        Mesh getColoredMesh(const Mesh& original) const;
    };

    ThicknessAnalyzer(const Mesh& mesh);
    ~ThicknessAnalyzer();

    Result computeProjectedThickness() const;

private:
    const Mesh& mesh;
    Scene scene;
    float meshDiag;
    void buildScene();
};

#endif // THICKNESS_ANALYZER_H
