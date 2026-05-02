#include "AutoOrientationOptimizer.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

AutoOrientationOptimizer::AutoOrientationOptimizer(const Mesh& mesh) : mesh(mesh) {
    buildScene();
    AABB bbox; for(const auto& v : mesh.vertices) bbox.expand(v);
    meshDiag = bbox.size().length();
}

AutoOrientationOptimizer::~AutoOrientationOptimizer() {}

void AutoOrientationOptimizer::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}

float AutoOrientationOptimizer::calculateSupportVolume(Vec3 upDir) const {
    float criticalCos = cos(135.0f * M_PI / 180.0f);
    float epsilon = meshDiag * 1e-4f;
    double totalVol = tbb::parallel_reduce(tbb::blocked_range<size_t>(0, mesh.triangles.size()), 0.0, [&](const auto& r, double init) {
        for(size_t i=r.begin(); i!=r.end(); ++i) {
            Vec3 n = computeFaceNormal(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
            if (n.x*upDir.x + n.y*upDir.y + n.z*upDir.z < criticalCos) {
                Vec3 center = computeFaceCenter(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
                Ray ray;
                ray.org = {center.x - n.x * epsilon, center.y - n.y * epsilon, center.z - n.z * epsilon};
                ray.dir = {-upDir.x, -upDir.y, -upDir.z};
                ray.tnear = 0.0f;
                ray.tfar = meshDiag * 2.0f;

                Hit hit = RayTracer::intersect(scene, ray);
                float dist = hit.hit ? hit.t : meshDiag;
                init += (double)dist * computeFaceArea(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
            }
        }
        return init;
    }, std::plus<double>());
    return (float)totalVol;
}

std::vector<AutoOrientationOptimizer::Result> AutoOrientationOptimizer::optimize(int numSamples) const {
    std::vector<Result> results;
    for(int s=0; s<numSamples; ++s) {
        float phi = acos(1.0f - 2.0f * (s + 0.5f) / numSamples);
        float theta = M_PI * (1.0f + sqrt(5.0f)) * (s + 0.5f);
        Vec3 up = {sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)};
        results.push_back({up, calculateSupportVolume(up)});
    }
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return a.supportVolume < b.supportVolume; });
    return results;
}
