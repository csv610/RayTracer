#include "SampleInterior.h"
#include <tbb/parallel_for.h>
#include <random>
#include <mutex>
#include <iostream>
#include <cstring>

SampleInterior::SampleInterior(const Mesh& mesh) : mesh(mesh) {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
    for (const auto& v : mesh.vertices) box.expand(v);
    buildScene();
}

SampleInterior::~SampleInterior() {
    if (scene) rtcReleaseScene(scene);
    if (device) rtcReleaseDevice(device);
}

void SampleInterior::buildScene() {
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

bool SampleInterior::isInside(const Vec3& p) const {
    RTCRayHit rh;
    rh.ray.org_x = p.x; rh.ray.org_y = p.y; rh.ray.org_z = p.z;
    rh.ray.dir_x = 0.0f; rh.ray.dir_y = 0.0f; rh.ray.dir_z = 1.0f;
    rh.ray.tnear = 0.0f;
    rh.ray.tfar = 1e10f;
    rh.ray.mask = -1;
    rh.ray.time = 0;
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    
    int intersections = 0;
    while (true) {
        rtcIntersect1(scene, &rh, &args);
        if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
        intersections++;
        rh.ray.org_x += rh.ray.dir_x * (rh.ray.tfar + 1e-4f);
        rh.ray.org_y += rh.ray.dir_y * (rh.ray.tfar + 1e-4f);
        rh.ray.org_z += rh.ray.dir_z * (rh.ray.tfar + 1e-4f);
        rh.ray.tnear = 0.0f;
        rh.ray.tfar = 1e10f;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }
    return (intersections % 2 != 0);
}

std::vector<Vec3> SampleInterior::sample(int numSamples) const {
    std::vector<Vec3> points;
    std::mutex mtx;
    int batchSize = 10000;

    while (points.size() < (size_t)numSamples) {
        tbb::parallel_for(0, batchSize, [&](int) {
            thread_local std::mt19937 gen(std::random_device{}());
            std::uniform_real_distribution<float> disX(box.min.x, box.max.x);
            std::uniform_real_distribution<float> disY(box.min.y, box.max.y);
            std::uniform_real_distribution<float> disZ(box.min.z, box.max.z);

            Vec3 p = {disX(gen), disY(gen), disZ(gen)};
            if (isInside(p)) {
                std::lock_guard<std::mutex> lock(mtx);
                if (points.size() < (size_t)numSamples) {
                    points.push_back(p);
                }
            }
        });
        std::cout << "\rProgress: " << points.size() << " / " << numSamples << std::flush;
    }
    std::cout << std::endl;
    return points;
}
