#ifndef ASSEMBLY_ANALYZER_H
#define ASSEMBLY_ANALYZER_H

#include "mesh_utils.h"
#include <embree4/rtcore.h>
#include <vector>

class AssemblyAnalyzer {
public:
    struct Result {
        std::vector<Vec3> colors;
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
    std::vector<Vec3> analyzeVisibility(Vec3 viewerPos) const;

private:
    const Mesh& part;
    const Mesh& env;
    RTCDevice device;
    RTCScene envScene;
    RTCScene combinedScene;

    void buildScenes();
    bool checkInside(RTCScene scene, const Vec3& p) const;
};

#endif
