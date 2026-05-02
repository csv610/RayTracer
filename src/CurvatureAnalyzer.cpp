#include "CurvatureAnalyzer.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <set>
#include <stdexcept>

CurvatureAnalyzer::CurvatureAnalyzer(const Mesh& m) : mesh(m) {
    buildScene();
    buildConnectivity();
    computeVertexNormals();
}

CurvatureAnalyzer::~CurvatureAnalyzer() {}

void CurvatureAnalyzer::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}

void CurvatureAnalyzer::buildConnectivity() {
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

void CurvatureAnalyzer::computeVertexNormals() {
    vertexNormals.resize(mesh.vertices.size(), {0, 0, 0});
    for (size_t t = 0; t < mesh.triangles.size(); ++t) {
        const auto& tri = mesh.triangles[t];
        Vec3 n = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        vertexNormals[tri.v0].x += n.x; vertexNormals[tri.v0].y += n.y; vertexNormals[tri.v0].z += n.z;
        vertexNormals[tri.v1].x += n.x; vertexNormals[tri.v1].y += n.y; vertexNormals[tri.v1].z += n.z;
        vertexNormals[tri.v2].x += n.x; vertexNormals[tri.v2].y += n.y; vertexNormals[tri.v2].z += n.z;
    }
    for (auto& n : vertexNormals) {
        float len = n.length();
        if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    }
}

float CurvatureAnalyzer::computeGaussianCurvature(unsigned int vidx) {
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

        float len1 = e1.length();
        float len2 = e2.length();
        if (len1 > 1e-6 && len2 > 1e-6) {
            float dot = (e1.x*e2.x + e1.y*e2.y + e1.z*e2.z) / (len1 * len2);
            dot = std::max(-1.0f, std::min(1.0f, dot));
            angleSum += acos(dot);
        }

        Vec3 cross = {e1.y*e2.z - e1.z*e2.y, e1.z*e2.x - e1.x*e2.z, e1.x*e2.y - e1.y*e2.x};
        float crossLen = cross.length();
        areaSum += crossLen * 0.5f;
    }

    if (areaSum < 1e-6) return 0.0f;
    return (2.0f * M_PI - angleSum) / areaSum;
}

float CurvatureAnalyzer::computeMeanCurvature(unsigned int vidx) {
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

            float len1 = e1.length();
            float len2 = e2.length();
            if (len1 < 1e-6 || len2 < 1e-6) continue;

            Vec3 cross = {e1.y*e2.z - e1.z*e2.y, e1.z*e2.x - e1.x*e2.z, e1.x*e2.y - e1.y*e2.x};
            float crossLen = cross.length();
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

float CurvatureAnalyzer::computeConcavity(unsigned int vidx, float avgEdgeLen) {
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
        float tlen = tangent.length();
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

            float rlen = rayDir.length();
            if (rlen > 1e-6) {
                rayDir.x /= rlen; rayDir.y /= rlen; rayDir.z /= rlen;
            }

            Ray ray;
            ray.org = {p0.x + n.x * avgEdgeLen * 0.01f, p0.y + n.y * avgEdgeLen * 0.01f, p0.z + n.z * avgEdgeLen * 0.01f};
            ray.dir = rayDir;
            ray.tnear = 0.0f;
            ray.tfar = avgEdgeLen * 5.0f;

            Hit hit = RayTracer::intersect(scene, ray);
            if (hit.hit) {
                if (hit.t > avgEdgeLen * 0.5f) {
                    concavityScore += 1.0f;
                }
                validRays++;
            }
        }
    }

    return (validRays > 0) ? (concavityScore / validRays) : 0.0f;
}

float CurvatureAnalyzer::computePocketDepth(unsigned int vidx, float avgEdgeLen) {
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

        float rlen = rayDir.length();
        if (rlen > 1e-6) {
            rayDir.x /= rlen; rayDir.y /= rlen; rayDir.z /= rlen;
        }

        Ray ray;
        ray.org = {p0.x + n.x * avgEdgeLen * 0.001f, p0.y + n.y * avgEdgeLen * 0.001f, p0.z + n.z * avgEdgeLen * 0.001f};
        ray.dir = rayDir;
        ray.tnear = 0.0f;
        ray.tfar = avgEdgeLen * 10.0f;

        Hit hit = RayTracer::intersect(scene, ray);
        if (hit.hit && hit.t < avgEdgeLen * 8.0f) {
            maxDist = std::max(maxDist, hit.t);
        }
    }

    return maxDist;
}
