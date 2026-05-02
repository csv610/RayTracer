#ifndef THICKNESS_ANALYZER_H
#define THICKNESS_ANALYZER_H

#include <vector>
#include "mesh_utils.h"
#include "RayTracer.h"

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
