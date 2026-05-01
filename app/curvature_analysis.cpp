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

struct CurvatureAnalyzer {
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    std::vector<std::vector<unsigned int>> vertexTriangles;
    std::vector<std::vector<unsigned int>> vertexNeighbors;
    std::vector<Vec3> vertexNormals;

    CurvatureAnalyzer(const Mesh& m) : mesh(m), device(nullptr), scene(nullptr) {
        device = rtcNewDevice(nullptr);
        if (!device) throw std::runtime_error("Failed to create Embree device");
        scene = rtcNewScene(device);
        buildScene();
        buildConnectivity();
        computeVertexNormals();
    }

    ~CurvatureAnalyzer() {
        if (scene) rtcReleaseScene(scene);
        if (device) rtcReleaseDevice(device);
    }

    void buildScene() {
        RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
        Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
        memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
        Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
        memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
        rtcCommitGeometry(geom);
        rtcAttachGeometry(scene, geom);
        rtcReleaseGeometry(geom);
        rtcCommitScene(scene);
    }

    void buildConnectivity() {
        vertexTriangles.resize(mesh.vertices.size());
        for (size_t t = 0; t < mesh.triangles.size(); ++t) {
            const auto& tri = mesh.triangles[t];
            vertexTriangles[tri.v0].push_back(t);
            vertexTriangles[tri.v1].push_back(t);
            vertexTriangles[tri.v2].push_back(t);
        }
        vertexNeighbors.resize(mesh.vertices.size());
        for (size_t v = 0; v < mesh.vertices.size(); ++v) {
            std::set<unsigned int> neighborSet;
            for (unsigned int t : vertexTriangles[v]) {
                const auto& tri = mesh.triangles[t];
                neighborSet.insert(tri.v0);
                neighborSet.insert(tri.v1);
                neighborSet.insert(tri.v2);
            }
            neighborSet.erase((unsigned int)v);
            vertexNeighbors[v].assign(neighborSet.begin(), neighborSet.end());
        }
    }

    void computeVertexNormals() {
        vertexNormals.resize(mesh.vertices.size(), {0, 0, 0});
        for (size_t t = 0; t < mesh.triangles.size(); ++t) {
            const auto& tri = mesh.triangles[t];
            Vec3 n = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
            vertexNormals[tri.v0].x += n.x; vertexNormals[tri.v0].y += n.y; vertexNormals[tri.v0].z += n.z;
            vertexNormals[tri.v1].x += n.x; vertexNormals[tri.v1].y += n.y; vertexNormals[tri.v1].z += n.z;
            vertexNormals[tri.v2].x += n.x; vertexNormals[tri.v2].y += n.y; vertexNormals[tri.v2].z += n.z;
        }
        for (auto& n : vertexNormals) {
            float len = sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
            if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
        }
    }

    float computeGaussianCurvature(unsigned int vidx) {
        const auto& neighbors = vertexNeighbors[vidx];
        if (neighbors.size() < 2) return 0.0f;

        Vec3 p0 = vertexToVec3(mesh.vertices[vidx]);
        float angleSum = 0.0f;
        float areaSum = 0.0f;

        for (unsigned int t : vertexTriangles[vidx]) {
            const auto& tri = mesh.triangles[t];
            unsigned int v0 = tri.v0, v1 = tri.v1, v2 = tri.v2;
            unsigned int other1 = (v0 == vidx) ? v1 : (v1 == vidx ? v2 : v0);
            unsigned int other2 = (v0 == vidx) ? v2 : (v1 == vidx ? v0 : v1);

            Vec3 p1 = vertexToVec3(mesh.vertices[other1]);
            Vec3 p2 = vertexToVec3(mesh.vertices[other2]);

            Vec3 e1 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
            Vec3 e2 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

            float len1 = sqrt(e1.x*e1.x + e1.y*e1.y + e1.z*e1.z);
            float len2 = sqrt(e2.x*e2.x + e2.y*e2.y + e2.z*e2.z);
            if (len1 > 1e-6 && len2 > 1e-6) {
                float dot = (e1.x*e2.x + e1.y*e2.y + e1.z*e2.z) / (len1 * len2);
                dot = std::max(-1.0f, std::min(1.0f, dot));
                angleSum += acos(dot);
            }

            Vec3 cross = {e1.y*e2.z - e1.z*e2.y, e1.z*e2.x - e1.x*e2.z, e1.x*e2.y - e1.y*e2.x};
            float crossLen = sqrt(cross.x*cross.x + cross.y*cross.y + cross.z*cross.z);
            areaSum += crossLen * 0.5f;
        }

        if (areaSum < 1e-6) return 0.0f;
        return (2.0f * M_PI - angleSum) / areaSum;
    }

    float computeMeanCurvature(unsigned int vidx) {
        const auto& neighbors = vertexNeighbors[vidx];
        if (neighbors.size() < 2) return 0.0f;

        Vec3 p0 = vertexToVec3(mesh.vertices[vidx]);
        Vec3 n = vertexNormals[vidx];
        float hSum = 0.0f;
        float weightSum = 0.0f;

        for (size_t i = 0; i + 1 < neighbors.size(); ++i) {
            for (size_t j = i + 1; j < neighbors.size(); ++j) {
                unsigned int v1 = neighbors[i];
                unsigned int v2 = neighbors[j];

                Vec3 p1 = vertexToVec3(mesh.vertices[v1]);
                Vec3 p2 = vertexToVec3(mesh.vertices[v2]);

                Vec3 e1 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
                Vec3 e2 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

                float len1 = sqrt(e1.x*e1.x + e1.y*e1.y + e1.z*e1.z);
                float len2 = sqrt(e2.x*e2.x + e2.y*e2.y + e2.z*e2.z);
                if (len1 < 1e-6 || len2 < 1e-6) continue;

                float crossLen = sqrt(pow(e1.y*e2.z - e1.z*e2.y, 2) +
                                      pow(e1.z*e2.x - e1.x*e2.z, 2) +
                                      pow(e1.x*e2.y - e1.y*e2.x, 2));
                if (crossLen < 1e-6) continue;

                float weight = crossLen / (len1 * len2);

                Vec3 n1 = vertexNormals[v1];
                Vec3 n2 = vertexNormals[v2];

                float curv1 = 1.0f - (n.x*n1.x + n.y*n1.y + n.z*n1.z);
                float curv2 = 1.0f - (n.x*n2.x + n.y*n2.y + n.z*n2.z);

                hSum += weight * (curv1 + curv2) * 0.5f;
                weightSum += weight;
            }
        }

        return (weightSum > 0) ? (hSum / weightSum) : 0.0f;
    }

    float computeConcavity(unsigned int vidx, float avgEdgeLen) {
        Vec3 p0 = vertexToVec3(mesh.vertices[vidx]);
        Vec3 n = vertexNormals[vidx];

        const int numRays = 16;
        float concavityScore = 0.0f;
        int validRays = 0;

        for (int i = 0; i < numRays; ++i) {
            float theta = 2.0f * M_PI * i / numRays;
            float phi = M_PI * 0.5f;

            Vec3 dir;
            dir.x = sin(phi) * cos(theta);
            dir.y = sin(phi) * sin(theta);
            dir.z = cos(phi);

            Vec3 tangent = {dir.y * n.z - dir.z * n.y, dir.z * n.x - dir.x * n.z, dir.x * n.y - dir.y * n.x};
            float tlen = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
            if (tlen > 1e-6) {
                tangent.x /= tlen; tangent.y /= tlen; tangent.z /= tlen;
            } else {
                tangent = {1, 0, 0};
            }

            Vec3 bitangent = {n.y * tangent.z - n.z * tangent.y, n.z * tangent.x - n.x * tangent.z, n.x * tangent.y - n.y * tangent.x};

            for (float angle = 0.2f; angle < M_PI; angle += 0.4f) {
                Vec3 rayDir;
                float c = cos(angle);
                float s = sin(angle);
                rayDir.x = tangent.x * s + bitangent.x * c + n.x * (1 - c);
                rayDir.y = tangent.y * s + bitangent.y * c + n.y * (1 - c);
                rayDir.z = tangent.z * s + bitangent.z * c + n.z * (1 - c);

                float rlen = sqrt(rayDir.x*rayDir.x + rayDir.y*rayDir.y + rayDir.z*rayDir.z);
                if (rlen > 1e-6) {
                    rayDir.x /= rlen; rayDir.y /= rlen; rayDir.z /= rlen;
                }

                RTCRayHit rh;
                rh.ray.org_x = p0.x + n.x * avgEdgeLen * 0.01f;
                rh.ray.org_y = p0.y + n.y * avgEdgeLen * 0.01f;
                rh.ray.org_z = p0.z + n.z * avgEdgeLen * 0.01f;
                rh.ray.dir_x = rayDir.x;
                rh.ray.dir_y = rayDir.y;
                rh.ray.dir_z = rayDir.z;
                rh.ray.tnear = 0.0f;
                rh.ray.tfar = avgEdgeLen * 5.0f;
                rh.ray.mask = -1;
                rh.ray.time = 0.0f;
                rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                rh.hit.primID = RTC_INVALID_GEOMETRY_ID;

                RTCIntersectArguments args;
                rtcInitIntersectArguments(&args);
                rtcIntersect1(scene, &rh, &args);

                if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                    float dist = rh.ray.tfar;
                    if (dist > avgEdgeLen * 0.5f) {
                        concavityScore += 1.0f;
                    }
                    validRays++;
                }
            }
        }

        return (validRays > 0) ? (concavityScore / validRays) : 0.0f;
    }

    float computePocketDepth(unsigned int vidx, float avgEdgeLen) {
        Vec3 p0 = vertexToVec3(mesh.vertices[vidx]);
        Vec3 n = vertexNormals[vidx];

        float maxDist = 0.0f;
        const int numRays = 8;

        for (int i = 0; i < numRays; ++i) {
            float angle = 2.0f * M_PI * i / numRays;

            float c = cos(angle);
            float s = sin(angle);

            Vec3 tangent = {1, 0, 0};
            if (fabs(n.x) < 0.9f) {
                tangent = {0, 1, 0};
            }
            Vec3 bitangent = {n.y * tangent.z - n.z * tangent.y, n.z * tangent.x - n.x * tangent.z, n.x * tangent.y - n.y * tangent.x};

            Vec3 rayDir;
            float spreadAngle = M_PI * 0.4f;
            float ca = cos(spreadAngle);
            float sa = sin(spreadAngle);
            rayDir.x = tangent.x * sa * c + bitangent.x * sa * s + n.x * ca;
            rayDir.y = tangent.y * sa * c + bitangent.y * sa * s + n.y * ca;
            rayDir.z = tangent.z * sa * c + bitangent.z * sa * s + n.z * ca;

            float rlen = sqrt(rayDir.x*rayDir.x + rayDir.y*rayDir.y + rayDir.z*rayDir.z);
            if (rlen > 1e-6) {
                rayDir.x /= rlen; rayDir.y /= rlen; rayDir.z /= rlen;
            }

            RTCRayHit rh;
            rh.ray.org_x = p0.x + n.x * avgEdgeLen * 0.001f;
            rh.ray.org_y = p0.y + n.y * avgEdgeLen * 0.001f;
            rh.ray.org_z = p0.z + n.z * avgEdgeLen * 0.001f;
            rh.ray.dir_x = rayDir.x;
            rh.ray.dir_y = rayDir.y;
            rh.ray.dir_z = rayDir.z;
            rh.ray.tnear = 0.0f;
            rh.ray.tfar = avgEdgeLen * 10.0f;
            rh.ray.mask = -1;
            rh.ray.time = 0.0f;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rh.hit.primID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args;
            rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID && rh.ray.tfar < avgEdgeLen * 8.0f) {
                maxDist = std::max(maxDist, rh.ray.tfar);
            }
        }

        return maxDist;
    }
};

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