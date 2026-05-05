#include "GeometryAnalyzer.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

GeometryAnalyzer::GeometryAnalyzer(const Mesh& mesh) : mesh(mesh) {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
    buildScene();
    MeshGeometry geom(mesh);
    meshDiag = geom.computeAABB().size().length();
}

GeometryAnalyzer::~GeometryAnalyzer() {
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
}

void GeometryAnalyzer::buildScene() {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Node* vb = (Node*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Node), mesh.nodes.size());
    memcpy(vb, mesh.nodes.data(), mesh.nodes.size() * sizeof(Node));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcCommitScene(scene);
}

MeshGeometry::Curvature GeometryAnalyzer::analyzeCurvature() const {
    MeshGeometry geom(mesh);
    return geom.computeCurvature();
}

std::vector<float> GeometryAnalyzer::runHemisphericalSampling(int samples, bool invert) const {
    std::vector<float> results(mesh.triangles.size());
    float epsilon = meshDiag * 1e-4f;
    float rayLength = meshDiag * 2.0f;

    tbb::parallel_for(size_t(0), mesh.triangles.size(), [&](size_t i) {
        Vec3 normal = MeshGeometry::computeFaceNormal(mesh.nodes[mesh.triangles[i].v0], mesh.nodes[mesh.triangles[i].v1], mesh.nodes[mesh.triangles[i].v2]);
        Vec3 center = MeshGeometry::computeFaceCenter(mesh.nodes[mesh.triangles[i].v0], mesh.nodes[mesh.triangles[i].v1], mesh.nodes[mesh.triangles[i].v2]);

        Vec3 up = (std::abs(normal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {normal.y * up.z - normal.z * up.y, normal.z * up.x - normal.x * up.z, normal.x * up.y - normal.y * up.x};
        float tLen = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
        tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen;
        Vec3 bitangent = {normal.y * tangent.z - normal.z * tangent.y, normal.z * tangent.x - normal.x * tangent.z, normal.x * tangent.y - normal.y * tangent.x};

        int hits = 0;
        for (int s = 0; s < samples; ++s) {
            float u = (float)s / samples;
            unsigned int bits = (s << 16) | (s >> 16);
            bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
            bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
            bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
            bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
            float v = (float)bits * 2.3283064365386963e-10;

            float phi = 2.0f * M_PI * u;
            float cosTheta = v;
            float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
            Vec3 localDir = {cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta};
            Vec3 rayDir = {
                tangent.x * localDir.x + bitangent.x * localDir.y + normal.x * localDir.z,
                tangent.y * localDir.x + bitangent.y * localDir.y + normal.y * localDir.z,
                tangent.z * localDir.x + bitangent.z * localDir.y + normal.z * localDir.z
            };

            RTCRayHit rh;
            rh.ray.org_x = center.x + normal.x * epsilon;
            rh.ray.org_y = center.y + normal.y * epsilon;
            rh.ray.org_z = center.z + normal.z * epsilon;
            rh.ray.dir_x = rayDir.x; rh.ray.dir_y = rayDir.y; rh.ray.dir_z = rayDir.z;
            rh.ray.tnear = 0; rh.ray.tfar = rayLength; rh.ray.mask = -1;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            RTCIntersectArguments args; rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);
            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) hits++;
        }
        float factor = (float)hits / samples;
        results[i] = invert ? (1.0f - factor) : factor;
    });
    return results;
}

std::vector<float> GeometryAnalyzer::computeAmbientOcclusion(int samples) const { return runHemisphericalSampling(samples, true); }
std::vector<float> GeometryAnalyzer::computeSkyViewFactor(int samples) const { return runHemisphericalSampling(samples, false); }
std::vector<float> GeometryAnalyzer::computePocketExposure(int samples) const { return runHemisphericalSampling(samples, false); }
