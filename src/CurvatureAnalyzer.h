#ifndef CURVATURE_ANALYZER_H
#define CURVATURE_ANALYZER_H

#include <vector>
#include <string>
#include "mesh_utils.h"
#include "RayTracer.h"

inline Vec3 vertexToVec3(const Vertex& v) { return {v.x, v.y, v.z}; }

struct CurvatureAnalyzer {
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
};

#endif // CURVATURE_ANALYZER_H
