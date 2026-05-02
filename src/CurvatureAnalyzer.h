#ifndef CURVATURE_ANALYZER_H
#define CURVATURE_ANALYZER_H

#include <vector>
#include <string>
#include "mesh_utils.h"
#include "RayTracer.h"

inline Vec3 vertexToVec3(const Vertex& v) { return {v.x, v.y, v.z}; }

struct CurvatureAnalyzer {
    struct Result {
        std::vector<float> gaussianCurv;
        std::vector<float> meanCurv;
        std::vector<float> concavity;
        std::vector<float> pocketDepth;
        std::vector<float> combinedScore;

        Mesh getColoredMesh(const Mesh& original) const;
    };

    const Mesh& mesh;
    Scene scene;
    std::vector<std::vector<unsigned int>> vertexTriangles;
    std::vector<std::vector<unsigned int>> vertexNeighbors;
    std::vector<Vec3> vertexNormals;

    CurvatureAnalyzer(const Mesh& m);
    ~CurvatureAnalyzer();

    void buildScene();
    void buildConnectivity();
    void computeVertexNormals();

    float computeGaussianCurvature(unsigned int vidx);
    float computeMeanCurvature(unsigned int vidx);
    float computeConcavity(unsigned int vidx, float avgEdgeLen);
    float computePocketDepth(unsigned int vidx, float avgEdgeLen);

    // High-level analysis
    Result analyze(float alpha = 0.4f, float beta = 0.3f, float gamma = 0.3f);
};

#endif // CURVATURE_ANALYZER_H
