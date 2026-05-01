#include "AutoOrientationOptimizer.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

AutoOrientationOptimizer::AutoOrientationOptimizer(const Mesh& mesh) : mesh(mesh) {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
    buildScene();
    AABB bbox; for(const auto& v : mesh.vertices) bbox.expand(v);
    meshDiag = bbox.size().length();
}

AutoOrientationOptimizer::~AutoOrientationOptimizer() {
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
}

void AutoOrientationOptimizer::buildScene() {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcCommitScene(scene);
}

float AutoOrientationOptimizer::calculateSupportVolume(Vec3 upDir) const {
    float criticalCos = cos(135.0f * M_PI / 180.0f);
    float epsilon = meshDiag * 1e-4f;
    double totalVol = tbb::parallel_reduce(tbb::blocked_range<size_t>(0, mesh.triangles.size()), 0.0, [&](const auto& r, double init) {
        for(size_t i=r.begin(); i!=r.end(); ++i) {
            Vec3 n = computeFaceNormal(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
            if (n.x*upDir.x + n.y*upDir.y + n.z*upDir.z < criticalCos) {
                Vec3 center = computeFaceCenter(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
                RTCRayHit rh; rh.ray.org_x=center.x-n.x*epsilon; rh.ray.org_y=center.y-n.y*epsilon; rh.ray.org_z=center.z-n.z*epsilon;
                rh.ray.dir_x=-upDir.x; rh.ray.dir_y=-upDir.y; rh.ray.dir_z=-upDir.z;
                rh.ray.tnear=0; rh.ray.tfar=meshDiag*2; rh.ray.mask=-1; rh.hit.geomID=RTC_INVALID_GEOMETRY_ID;
                RTCIntersectArguments args; rtcInitIntersectArguments(&args); rtcIntersect1(scene, &rh, &args);
                float dist = (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) ? rh.ray.tfar : meshDiag;
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
