#include "ManufacturingAnalyzer.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

ManufacturingAnalyzer::ManufacturingAnalyzer(const Mesh& mesh) : mesh(mesh) {
    buildScene();

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size();
    meshDiag = sqrt(size.x*size.x + size.y*size.y + size.z*size.z);
}

ManufacturingAnalyzer::~ManufacturingAnalyzer() {}

void ManufacturingAnalyzer::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}

ManufacturingAnalyzer::Result ManufacturingAnalyzer::analyzeUndercuts(Vec3 pullDir) const {
    Result res;
    res.colors.resize(mesh.triangles.size());
    float epsilon = meshDiag * 1e-4f;

    int ucCount = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, mesh.triangles.size()),
        0,
        [&](const tbb::blocked_range<size_t>& r, int init) -> int {
            int localCount = init;
            for (size_t i = r.begin(); i != r.end(); ++i) {
                const auto& tri = mesh.triangles[i];
                Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
                Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

                float dot = normal.x * pullDir.x + normal.y * pullDir.y + normal.z * pullDir.z;
                bool undercut = false;
                if (dot < -0.01f) {
                    undercut = true;
                } else {
                    Ray ray;
                    ray.org = {faceCenter.x + normal.x * epsilon, faceCenter.y + normal.y * epsilon, faceCenter.z + normal.z * epsilon};
                    ray.dir = pullDir;
                    ray.tnear = 0.0f;
                    ray.tfar = meshDiag * 2.0f;
                    
                    if (RayTracer::occluded(scene, ray)) undercut = true;
                }
                if (undercut) {
                    res.colors[i] = {255, 0, 0, 255}; localCount++;
                } else {
                    res.colors[i] = {0, 255, 0, 255};
                }
            }
            return localCount;
        }, std::plus<int>()
    );
    res.count = ucCount;
    return res;
}

ManufacturingAnalyzer::Result ManufacturingAnalyzer::analyzeOverhangs(float thresholdDeg) const {
    Result res;
    res.colors.resize(mesh.triangles.size());
    float criticalCos = cos((180.0f - thresholdDeg) * M_PI / 180.0f);

    int ohCount = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, mesh.triangles.size()),
        0,
        [&](const tbb::blocked_range<size_t>& r, int init) -> int {
            int localCount = init;
            for (size_t i = r.begin(); i != r.end(); ++i) {
                Vec3 normal = computeFaceNormal(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
                if (normal.z < criticalCos) {
                    float t = std::clamp((normal.z - (-1.0f)) / (criticalCos - (-1.0f)), 0.0f, 1.0f);
                    res.colors[i] = {(unsigned char)(255.0f), (unsigned char)(t * 255.0f), 0, 255}; localCount++;
                } else {
                    res.colors[i] = {0, 255, 0, 255};
                }
            }
            return localCount;
        }, std::plus<int>()
    );
    res.count = ohCount;
    return res;
}

ManufacturingAnalyzer::Result ManufacturingAnalyzer::analyzeDraftAngles(Vec3 pullDir) const {
    Result res;
    res.colors.resize(mesh.triangles.size());
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        Vec3 normal = computeFaceNormal(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
        float dot = std::clamp(normal.x * pullDir.x + normal.y * pullDir.y + normal.z * pullDir.z, -1.0f, 1.0f);
        float angleDeg = (M_PI * 0.5f - acos(dot)) * 180.0f / M_PI;
        if (angleDeg < -0.1f) res.colors[i] = {255, 0, 0, 255};
        else if (angleDeg < 3.0f) res.colors[i] = {(unsigned char)(255.0f), (unsigned char)(std::max(0.0f, angleDeg/3.0f) * 255.0f), 0, 255};
        else res.colors[i] = {0, 255, 0, 255};
    }
    return res;
}

float ManufacturingAnalyzer::calculateUndercutScore(Vec3 pullDir) const {
    return (float)analyzeUndercuts(pullDir).count / mesh.triangles.size();
}

Vec3 ManufacturingAnalyzer::findOptimalPartingLine(int numSamples) const {
    float bestScore = 1e10f;
    Vec3 bestDir = {0, 0, 1};
    for (int s = 0; s < numSamples; ++s) {
        float phi = acos(1.0f - 2.0f * (s + 0.5f) / numSamples);
        float theta = M_PI * (1.0f + sqrt(5.0f)) * (s + 0.5f);
        Vec3 dir = {sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)};
        float score = calculateUndercutScore(dir);
        if (score < bestScore) { bestScore = score; bestDir = dir; }
    }
    return bestDir;
}
