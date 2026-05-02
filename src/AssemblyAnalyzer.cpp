#include "AssemblyAnalyzer.h"
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
        Vec3 center = computeFaceCenter(part.vertices[part.triangles[i].v0], part.vertices[part.triangles[i].v1], part.vertices[part.triangles[i].v2]);
        Vec3 normal = computeFaceNormal(part.vertices[part.triangles[i].v0], part.vertices[part.triangles[i].v1], part.vertices[part.triangles[i].v2]);
        
        if (envScene.isInside(center)) { res.colors[i] = {1,0,0}; return; }
        
        Ray ray;
        ray.org = center;
        ray.dir = normal;
        ray.tnear = 0.0f;
        ray.tfar = threshold;
        
        if (RayTracer::occluded(envScene, ray)) res.colors[i] = {1, 0.5f, 0};
        else res.colors[i] = {0, 1, 0};
    });
    for(const auto& c : res.colors) { if(c.y == 0 && c.x == 1) res.collisions++; else if(c.y == 0.5f) res.violations++; }
    return res;
}

AssemblyAnalyzer::Result AssemblyAnalyzer::verifyExtractionPath(Vec3 moveDir, float distance) const {
    Result res;
    res.colors.resize(part.triangles.size());
    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        Vec3 center = computeFaceCenter(part.vertices[part.triangles[i].v0], part.vertices[part.triangles[i].v1], part.vertices[part.triangles[i].v2]);
        Ray ray;
        ray.org = center;
        ray.dir = moveDir;
        ray.tnear = 0.0f;
        ray.tfar = distance;
        
        if (RayTracer::occluded(envScene, ray)) { res.colors[i] = {1,0,0}; }
        else res.colors[i] = {0,1,0};
    });
    for(const auto& c : res.colors) if(c.x > 0.5f) res.collisions++;
    return res;
}

std::vector<Vec3> AssemblyAnalyzer::analyzeVisibility(Vec3 viewerPos) const {
    std::vector<Vec3> colors(part.triangles.size());
    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        Vec3 center = computeFaceCenter(part.vertices[part.triangles[i].v0], part.vertices[part.triangles[i].v1], part.vertices[part.triangles[i].v2]);
        Vec3 dir = {center.x - viewerPos.x, center.y - viewerPos.y, center.z - viewerPos.z};
        float dist = dir.length();
        if(dist > 0) { dir.x/=dist; dir.y/=dist; dir.z/=dist; }
        
        Ray ray;
        ray.org = viewerPos;
        ray.dir = dir;
        ray.tnear = 0.0f;
        ray.tfar = dist * 1.01f;
        
        Hit hit = RayTracer::intersect(combinedScene, ray);
        
        if (hit.hit && hit.geomID == 0 && std::abs(hit.t - dist) < dist * 0.05f) colors[i] = {0,1,0};
        else colors[i] = {1,0,0};
    });
    return colors;
}
