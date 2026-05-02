#include "CurvatureAnalyzer.h"
#include "MeshIO.h"
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "curvature_output.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    try {
        CurvatureAnalyzer analyzer(mesh);

        auto dist = [](const Vertex& a, const Vertex& b) {
            float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
            return sqrt(dx*dx + dy*dy + dz*dz);
        };
        float avgEdgeLen = 0.0f;
        for (const auto& tri : mesh.triangles) {
            avgEdgeLen += dist(mesh.vertices[tri.v0], mesh.vertices[tri.v1]) + 
                          dist(mesh.vertices[tri.v1], mesh.vertices[tri.v2]) + 
                          dist(mesh.vertices[tri.v0], mesh.vertices[tri.v2]);
        }
        avgEdgeLen /= (mesh.triangles.size() * 3);

        std::vector<float> gaussianCurv(mesh.vertices.size());
        std::vector<float> meanCurv(mesh.vertices.size());
        std::vector<float> concavity(mesh.vertices.size());
        std::vector<float> pocketDepth(mesh.vertices.size());

        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            gaussianCurv[i] = analyzer.computeGaussianCurvature((unsigned int)i);
            meanCurv[i] = analyzer.computeMeanCurvature((unsigned int)i);
            concavity[i] = analyzer.computeConcavity((unsigned int)i, avgEdgeLen);
            pocketDepth[i] = analyzer.computePocketDepth((unsigned int)i, avgEdgeLen);
        }

        float minG = *std::min_element(gaussianCurv.begin(), gaussianCurv.end());
        float maxG = *std::max_element(gaussianCurv.begin(), gaussianCurv.end());
        float minM = *std::min_element(meanCurv.begin(), meanCurv.end());
        float maxM = *std::max_element(meanCurv.begin(), meanCurv.end());
        float minC = *std::min_element(concavity.begin(), concavity.end());
        float maxC = *std::max_element(concavity.begin(), concavity.end());
        float minP = *std::min_element(pocketDepth.begin(), pocketDepth.end());
        float maxP = *std::max_element(pocketDepth.begin(), pocketDepth.end());

        float alpha = 0.4f, beta = 0.3f, gamma = 0.3f;
        std::vector<float> combinedScore(mesh.vertices.size());
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            float gNorm = (maxG != minG) ? (gaussianCurv[i] - minG) / (maxG - minG) : 0.5f;
            float mNorm = (maxM != minM) ? (meanCurv[i] - minM) / (maxM - minM) : 0.5f;
            float cNorm = (maxC != minC) ? (concavity[i] - minC) / (maxC - minC) : 0.5f;
            float pNorm = (maxP != minP) ? (pocketDepth[i] - minP) / (maxP - minP) : 0.5f;
            combinedScore[i] = alpha * (gNorm + mNorm) * 0.5f + beta * cNorm + gamma * pNorm;
        }

        float minS = *std::min_element(combinedScore.begin(), combinedScore.end());
        float maxS = *std::max_element(combinedScore.begin(), combinedScore.end());

        mesh.faceColors.resize(mesh.triangles.size());
        for (size_t i = 0; i < mesh.triangles.size(); ++i) {
            const auto& tri = mesh.triangles[i];
            float avgScore = (combinedScore[tri.v0] + combinedScore[tri.v1] + combinedScore[tri.v2]) / 3.0f;
            float t = (maxS != minS) ? (avgScore - minS) / (maxS - minS) : 0.5f;
            mesh.faceColors[i] = getJetColor(t);
        }
        MeshIO::save(outputFile, mesh);
        printf("Curvature analysis saved to %s\n", outputFile.c_str());

    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    return 0;
}
