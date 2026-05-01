#include "AssemblyAnalyzer.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

AssemblyAnalyzer::AssemblyAnalyzer(const Mesh& part, const Mesh& environment) : part(part), env(environment) {
    device = rtcNewDevice(nullptr);
    envScene = rtcNewScene(device);
    combinedScene = rtcNewScene(device);
    buildScenes();
}

AssemblyAnalyzer::~AssemblyAnalyzer() {
    rtcReleaseScene(envScene);
    rtcReleaseScene(combinedScene);
    rtcReleaseDevice(device);
}

void AssemblyAnalyzer::buildScenes() {
    // Environment only scene
    RTCGeometry gEnv = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vbE = (Vertex*)rtcSetNewGeometryBuffer(gEnv, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), env.vertices.size());
    memcpy(vbE, env.vertices.data(), env.vertices.size() * sizeof(Vertex));
    Triangle* ibE = (Triangle*)rtcSetNewGeometryBuffer(gEnv, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), env.triangles.size());
    memcpy(ibE, env.triangles.data(), env.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(gEnv);
    rtcAttachGeometry(envScene, gEnv);
    rtcCommitScene(envScene);

    // Combined scene (Part = 0, Env = 1)
    RTCGeometry gPart = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vbP = (Vertex*)rtcSetNewGeometryBuffer(gPart, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), part.vertices.size());
    memcpy(vbP, part.vertices.data(), part.vertices.size() * sizeof(Vertex));
    Triangle* ibP = (Triangle*)rtcSetNewGeometryBuffer(gPart, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), part.triangles.size());
    memcpy(ibP, part.triangles.data(), part.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(gPart);
    rtcAttachGeometry(combinedScene, gPart); // ID 0

    RTCGeometry gEnvC = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    rtcSetSharedGeometryBuffer(gEnvC, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, env.vertices.data(), 0, sizeof(Vertex), env.vertices.size());
    rtcSetSharedGeometryBuffer(gEnvC, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, env.triangles.data(), 0, sizeof(Triangle), env.triangles.size());
    rtcCommitGeometry(gEnvC);
    rtcAttachGeometry(combinedScene, gEnvC); // ID 1
    rtcCommitScene(combinedScene);
}

bool AssemblyAnalyzer::checkInside(RTCScene scene, const Vec3& p) const {
    RTCRayHit rh;
    rh.ray.org_x = p.x; rh.ray.org_y = p.y; rh.ray.org_z = p.z;
    rh.ray.dir_x = 0.314f; rh.ray.dir_y = 0.718f; rh.ray.dir_z = 0.941f;
    rh.ray.tnear = 0; rh.ray.tfar = 1e10f; rh.ray.mask = -1;
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    RTCIntersectArguments args; rtcInitIntersectArguments(&args);
    int hits = 0;
    while(true) {
        rtcIntersect1(scene, &rh, &args);
        if(rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
        hits++;
        rh.ray.tnear = rh.ray.tfar + 1e-4f;
        rh.ray.tfar = 1e10f;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }
    return (hits % 2 != 0);
}

AssemblyAnalyzer::Result AssemblyAnalyzer::analyzeClearance(float threshold) const {
    Result res;
    res.colors.resize(part.triangles.size());
    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        Vec3 center = computeFaceCenter(part.vertices[part.triangles[i].v0], part.vertices[part.triangles[i].v1], part.vertices[part.triangles[i].v2]);
        Vec3 normal = computeFaceNormal(part.vertices[part.triangles[i].v0], part.vertices[part.triangles[i].v1], part.vertices[part.triangles[i].v2]);
        if (checkInside(envScene, center)) { res.colors[i] = {1,0,0}; return; }
        RTCRayHit rh;
        rh.ray.org_x = center.x; rh.ray.org_y = center.y; rh.ray.org_z = center.z;
        rh.ray.dir_x = normal.x; rh.ray.dir_y = normal.y; rh.ray.dir_z = normal.z;
        rh.ray.tnear = 0; rh.ray.tfar = threshold; rh.ray.mask = -1;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        RTCIntersectArguments args; rtcInitIntersectArguments(&args);
        rtcIntersect1(envScene, &rh, &args);
        if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) res.colors[i] = {1, 0.5f, 0};
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
        RTCRayHit rh;
        rh.ray.org_x = center.x; rh.ray.org_y = center.y; rh.ray.org_z = center.z;
        rh.ray.dir_x = moveDir.x; rh.ray.dir_y = moveDir.y; rh.ray.dir_z = moveDir.z;
        rh.ray.tnear = 0; rh.ray.tfar = distance; rh.ray.mask = -1;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        RTCIntersectArguments args; rtcInitIntersectArguments(&args);
        rtcIntersect1(envScene, &rh, &args);
        if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) { res.colors[i] = {1,0,0}; }
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
        float dist = sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        if(dist > 0) { dir.x/=dist; dir.y/=dist; dir.z/=dist; }
        RTCRayHit rh;
        rh.ray.org_x = viewerPos.x; rh.ray.org_y = viewerPos.y; rh.ray.org_z = viewerPos.z;
        rh.ray.dir_x = dir.x; rh.ray.dir_y = dir.y; rh.ray.dir_z = dir.z;
        rh.ray.tnear = 0; rh.ray.tfar = dist * 1.01f; rh.ray.mask = -1;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        RTCIntersectArguments args; rtcInitIntersectArguments(&args);
        rtcIntersect1(combinedScene, &rh, &args);
        if (rh.hit.geomID == 0 && std::abs(rh.ray.tfar - dist) < dist * 0.05f) colors[i] = {0,1,0};
        else colors[i] = {1,0,0};
    });
    return colors;
}
