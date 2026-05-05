#include "AssemblyAnalyzer.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

AssemblyAnalyzer::AssemblyAnalyzer(const Mesh& part, const Mesh& environment) : part(part), env(environment) {
    buildScenes();
}

AssemblyAnalyzer::~AssemblyAnalyzer() {}

void AssemblyAnalyzer::buildScenes() {
    envScene.addSharedMesh(env);
    envScene.commit();

    combinedScene.addSharedMesh(part); // ID 0
    combinedScene.addSharedMesh(env);  // ID 1
    combinedScene.commit();
}

AssemblyAnalyzer::Result AssemblyAnalyzer::analyzeClearance(float threshold) const {
    Result res;
    res.colors.resize(part.triangles.size());
    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        Vec3 center = MeshGeometry::computeFaceCenter(part.nodes[part.triangles[i].v0], part.nodes[part.triangles[i].v1], part.nodes[part.triangles[i].v2]);
        Vec3 normal = MeshGeometry::computeFaceNormal(part.nodes[part.triangles[i].v0], part.nodes[part.triangles[i].v1], part.nodes[part.triangles[i].v2]);
        
        if (envScene.isInside(center)) { res.colors[i] = {255, 0, 0, 255}; return; }
        
        Ray ray;
        ray.org = center;
        ray.dir = normal;
        ray.tnear = 0.0f;
        ray.tfar = threshold;
        
        if (RayTracer::occluded(envScene, ray)) res.colors[i] = {255, 128, 0, 255};
        else res.colors[i] = {0, 255, 0, 255};
    });
    for(const auto& c : res.colors) { if(c.g == 0 && c.r == 255) res.collisions++; else if(c.g == 128) res.violations++; }
    return res;
}

AssemblyAnalyzer::Result AssemblyAnalyzer::verifyExtractionPath(Vec3 moveDir, float distance) const {
    Result res;
    res.colors.resize(part.triangles.size());
    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        Vec3 center = MeshGeometry::computeFaceCenter(part.nodes[part.triangles[i].v0], part.nodes[part.triangles[i].v1], part.nodes[part.triangles[i].v2]);
        Ray ray;
        ray.org = center;
        ray.dir = moveDir;
        ray.tnear = 0.0f;
        ray.tfar = distance;
        
        if (RayTracer::occluded(envScene, ray)) { res.colors[i] = {255, 0, 0, 255}; }
        else res.colors[i] = {0, 255, 0, 255};
    });
    for(const auto& c : res.colors) if(c.r > 128) res.collisions++;
    return res;
}

std::vector<Color4b> AssemblyAnalyzer::analyzeVisibility(Vec3 viewerPos) const {
    std::vector<Color4b> colors(part.triangles.size());
    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        Vec3 center = MeshGeometry::computeFaceCenter(part.nodes[part.triangles[i].v0], part.nodes[part.triangles[i].v1], part.nodes[part.triangles[i].v2]);
        Vec3 dir = {center.x - viewerPos.x, center.y - viewerPos.y, center.z - viewerPos.z};
        float dist = dir.length();
        if(dist > 0) { dir.x/=dist; dir.y/=dist; dir.z/=dist; }
        
        Ray ray;
        ray.org = viewerPos;
        ray.dir = dir;
        ray.tnear = 0.0f;
        ray.tfar = dist * 1.01f;
        
        Hit hit = RayTracer::intersect(combinedScene, ray);
        
        if (hit.hit && hit.geomID == 0 && std::abs(hit.t - dist) < dist * 0.05f) colors[i] = {0, 255, 0, 255};
        else colors[i] = {255, 0, 0, 255};
    });
    return colors;
}
