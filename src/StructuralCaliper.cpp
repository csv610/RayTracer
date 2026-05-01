#include "StructuralCaliper.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <algorithm>
#include <cstring>

StructuralCaliper::StructuralCaliper(const Mesh& mesh) : mesh(mesh) {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
    buildScene();
}

StructuralCaliper::~StructuralCaliper() {
    if (scene) rtcReleaseScene(scene);
    if (device) rtcReleaseDevice(device);
}

void StructuralCaliper::buildScene() {
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

std::vector<StructuralCaliper::AnalysisResult> StructuralCaliper::analyze(int numSamples, float threshold) const {
    SampleSurface sampler(mesh);
    std::vector<SampledPoint> surfacePoints = sampler.sample(numSamples, 0);

    std::vector<AnalysisResult> results(surfacePoints.size());
    std::cout << "Running structural analysis on " << surfacePoints.size() << " samples..." << std::endl;

    tbb::parallel_for(size_t(0), surfacePoints.size(), [&](size_t i) {
        const auto& sp = surfacePoints[i];
        
        RTCRayHit rh;
        float epsilon = 1e-4f;
        rh.ray.org_x = sp.p.x - sp.n.x * epsilon;
        rh.ray.org_y = sp.p.y - sp.n.y * epsilon;
        rh.ray.org_z = sp.p.z - sp.n.z * epsilon;
        rh.ray.dir_x = -sp.n.x;
        rh.ray.dir_y = -sp.n.y;
        rh.ray.dir_z = -sp.n.z;
        rh.ray.tnear = 0.0f;
        rh.ray.tfar = 1e10f;
        rh.ray.mask = -1;
        rh.ray.time = 0;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

        RTCIntersectArguments args;
        rtcInitIntersectArguments(&args);
        rtcIntersect1(scene, &rh, &args);

        float thickness = (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) ? rh.ray.tfar : 1e10f;
        
        Vec3 color;
        if (thickness < threshold) {
            float t = std::clamp(thickness / threshold, 0.0f, 1.0f);
            color = {1.0f, t, 0.0f}; // Red (0) to Yellow (threshold)
        } else {
            color = {0.0f, 1.0f, 0.0f}; // Green (Safe)
        }
        
        results[i] = {sp.p, thickness, color};
    });

    return results;
}
