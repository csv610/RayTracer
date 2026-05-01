#include "CurvatureAnalyzer.h"
#include <embree4/rtcore.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <limits>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

inline Vec3 vertexToVec3(const Vertex& v) {
    return {v.x, v.y, v.z};
}


int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        printf("Analyzes surface curvature and detects concave/convex regions.\n");
        printf("Output: convex regions (red/orange), concave regions (blue/cyan)\n");
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "curvature_output.off";

    printf("Loading mesh: %s\n", inputFile.c_str());
    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    printf("Mesh: %zu vertices, %zu triangles\n", mesh.vertices.size(), mesh.triangles.size());

    try {
        CurvatureAnalyzer analyzer(mesh);

        auto dist = [](const Vertex& a, const Vertex& b) {
            float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
            return sqrt(dx*dx + dy*dy + dz*dz);
        };
        float avgEdgeLen = 0.0f;
        for (const auto& tri : mesh.triangles) {
            const Vertex& v0 = mesh.vertices[tri.v0];
            const Vertex& v1 = mesh.vertices[tri.v1];
            const Vertex& v2 = mesh.vertices[tri.v2];
            avgEdgeLen += dist(v0, v1) + dist(v1, v2) + dist(v0, v2);
        }
        avgEdgeLen /= (mesh.triangles.size() * 3);
        printf("Average edge length: %.4f\n", avgEdgeLen);

        printf("Computing curvatures...\n");
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

        printf("Gaussian curvature range: [%.4f, %.4f]\n", minG, maxG);
        printf("Mean curvature range: [%.4f, %.4f]\n", minM, maxM);
        printf("Concavity range: [%.4f, %.4f]\n", minC, maxC);
        printf("Pocket depth range: [%.4f, %.4f]\n", minP, maxP);

        float alpha = 0.4f;
        float beta = 0.3f;
        float gamma = 0.3f;

        std::vector<float> combinedScore(mesh.vertices.size());
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            float gNorm = (maxG != minG) ? (gaussianCurv[i] - minG) / (maxG - minG) : 0.5f;
            float mNorm = (maxM != minM) ? (meanCurv[i] - minM) / (maxM - minM) : 0.5f;
            float cNorm = (maxC != minC) ? (concavity[i] - minC) / (maxC - minC) : 0.5f;
            float pNorm = (maxP != minP) ? (pocketDepth[i] - minP) / (maxP - minP) : 0.5f;

            float curvScore = (gNorm + mNorm) * 0.5f;
            combinedScore[i] = alpha * curvScore + beta * cNorm + gamma * pNorm;
        }

        float minS = *std::min_element(combinedScore.begin(), combinedScore.end());
        float maxS = *std::max_element(combinedScore.begin(), combinedScore.end());

        FILE* out = fopen(outputFile.c_str(), "w");
        fprintf(out, "OFF\n");
        fprintf(out, "%zu %zu 0\n", mesh.vertices.size(), mesh.triangles.size());
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            fprintf(out, "%.6f %.6f %.6f\n", mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
        }
        for (size_t i = 0; i < mesh.triangles.size(); ++i) {
            const auto& tri = mesh.triangles[i];
            float avgScore = (combinedScore[tri.v0] + combinedScore[tri.v1] + combinedScore[tri.v2]) / 3.0f;
            float t = (maxS != minS) ? (avgScore - minS) / (maxS - minS) : 0.5f;
            Vec3 color = getJetColor(t);
            fprintf(out, "3 %d %d %d %.6f %.6f %.6f\n", tri.v0, tri.v1, tri.v2, color.x, color.y, color.z);
        }
        fclose(out);
        printf("Output written to %s\n", outputFile.c_str());

        int convexCount = 0, concaveCount = 0, flatCount = 0;
        for (size_t i = 0; i < combinedScore.size(); ++i) {
            float t = (maxS != minS) ? (combinedScore[i] - minS) / (maxS - minS) : 0.5f;
            if (t < 0.33f) concaveCount++;
            else if (t > 0.66f) convexCount++;
            else flatCount++;
        }
        printf("Region distribution: convex=%d, flat=%d, concave=%d\n", convexCount, flatCount, concaveCount);

    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }

    return 0;
}
