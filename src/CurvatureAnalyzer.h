#ifndef CURVATURE_ANALYZER_H
#define CURVATURE_ANALYZER_H

#include <embree4/rtcore.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <set>
#include <stdexcept>
#include <tbb/parallel_for.h>
#include "mesh_utils.h"

inline Vec3 vertexToVec3(const Vertex& v) { return {v.x, v.y, v.z}; }

struct CurvatureAnalyzer {
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
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
