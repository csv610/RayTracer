#ifndef ASSEMBLY_ANALYZER_H
#define ASSEMBLY_ANALYZER_H

#include "mesh_utils.h"
#include "RayTracer.h"
#include <vector>

class AssemblyAnalyzer {
public:
    struct Result {
        std::vector<Color4b> colors;
        int collisions = 0;
        int violations = 0;
        Mesh getColoredMesh(const Mesh& original) const {
            Mesh m = original;
            m.faceColors = colors;
            return m;
        }
    };

    AssemblyAnalyzer(const Mesh& part, const Mesh& environment);
    ~AssemblyAnalyzer();

    Result analyzeClearance(float threshold) const;
    Result verifyExtractionPath(Vec3 moveDir, float distance) const;
    std::vector<Color4b> analyzeVisibility(Vec3 viewerPos) const;

private:
    const Mesh& part;
    const Mesh& env;
    Scene envScene;
    Scene combinedScene;

    void buildScenes();
};

#endif
