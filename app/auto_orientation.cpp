#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include "mesh_utils.h"
#include "MeshIO.h"

struct OrientationResult {
    Vec3 up;
    float supportVolume;
};

// Rotates a vector p around axis by angle theta
Vec3 rotateVector(const Vec3& p, const Vec3& axis, float theta) {
    float cosT = cos(theta);
    float sinT = sin(theta);
    // Rodrigues' rotation formula
    float dot = p.x*axis.x + p.y*axis.y + p.z*axis.z;
    Vec3 cross = {axis.y*p.z - axis.z*p.y, axis.z*p.x - axis.x*p.z, axis.x*p.y - axis.y*p.x};
    return {
        p.x*cosT + cross.x*sinT + axis.x*dot*(1-cosT),
        p.y*cosT + cross.y*sinT + axis.y*dot*(1-cosT),
        p.z*cosT + cross.z*sinT + axis.z*dot*(1-cosT)
    };
}

// Calculates support volume for a given 'up' direction
float calculateSupportVolume(RTCScene scene, const Mesh& mesh, const Vec3& upDir, float meshDiag) {
    // We need a local frame where upDir is +Z
    Vec3 z = upDir;
    Vec3 x = (std::abs(z.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
    Vec3 y = {z.y*x.z - z.z*x.y, z.z*x.x - z.x*x.z, z.x*x.y - z.y*x.x};
    float yLen = sqrt(y.x*y.x + y.y*y.y + y.z*y.z);
    y.x /= yLen; y.y /= yLen; y.z /= yLen;
    x = {y.y*z.z - y.z*z.y, y.z*z.x - y.x*z.z, y.x*z.y - y.y*z.x};

    float criticalCos = cos(135.0f * M_PI / 180.0f); // 45 deg overhang
    float epsilon = meshDiag * 1e-4f;

    double totalVol = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, mesh.triangles.size()),
        0.0,
        [&](const tbb::blocked_range<size_t>& r, double init) -> double {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                const auto& tri = mesh.triangles[i];
                const auto& v0 = mesh.vertices[tri.v0];
                const auto& v1 = mesh.vertices[tri.v1];
                const auto& v2 = mesh.vertices[tri.v2];
                
                Vec3 n = computeFaceNormal(v0, v1, v2);
                float dot = n.x*upDir.x + n.y*upDir.y + n.z*upDir.z;
                
                if (dot < criticalCos) {
                    Vec3 center = computeFaceCenter(v0, v1, v2);
                    Vec3 e1 = {v1.x-v0.x, v1.y-v0.y, v1.z-v0.z};
                    Vec3 e2 = {v2.x-v0.x, v2.y-v0.y, v2.z-v0.z};
                    Vec3 cross = {e1.y*e2.z-e1.z*e2.y, e1.z*e2.x-e1.x*e2.z, e1.x*e2.y-e1.y*e2.x};
                    float area = 0.5f * sqrt(cross.x*cross.x + cross.y*cross.y + cross.z*cross.z);

                    // Cast ray in -upDir
                    RTCRayHit rh;
                    rh.ray.org_x = center.x - n.x * epsilon;
                    rh.ray.org_y = center.y - n.y * epsilon;
                    rh.ray.org_z = center.z - n.z * epsilon;
                    rh.ray.dir_x = -upDir.x; rh.ray.dir_y = -upDir.y; rh.ray.dir_z = -upDir.z;
                    rh.ray.tnear = 0.0f; rh.ray.tfar = meshDiag * 2.0f; rh.ray.mask = -1;
                    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    RTCIntersectArguments args; rtcInitIntersectArguments(&args);
                    
                    rtcIntersect1(scene, &rh, &args);
                    float dist = (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) ? rh.ray.tfar : meshDiag;
                    init += (double)dist * area;
                }
            }
            return init;
        },
        std::plus<double>()
    );
    return (float)totalVol;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [num_samples]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int numSamples = (argc >= 3) ? std::atoi(argv[2]) : 32;

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene);

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    float diag = sqrt(pow(bbox.max.x-bbox.min.x,2) + pow(bbox.max.y-bbox.min.y,2) + pow(bbox.max.z-bbox.min.z,2));

    std::cout << "Searching for optimal build orientation (" << numSamples << " directions)..." << std::endl;

    std::vector<OrientationResult> results;
    for (int s = 0; s < numSamples; ++s) {
        float phi = acos(1.0f - 2.0f * (s + 0.5f) / numSamples);
        float theta = M_PI * (1.0f + sqrt(5.0f)) * (s + 0.5f);
        Vec3 up = {sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)};
        
        float vol = calculateSupportVolume(scene, mesh, up, diag);
        results.push_back({up, vol});
        
        if (s % 10 == 0) std::cout << "Progress: " << (s*100/numSamples) << "%" << std::endl;
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.supportVolume < b.supportVolume;
    });

    std::cout << "\nOptimal Build Orientation Analysis:" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    for (int i = 0; i < std::min(5, (int)results.size()); ++i) {
        std::cout << "Rank " << i+1 << ": UpDirection(" << results[i].up.x << ", " << results[i].up.y << ", " << results[i].up.z 
                  << "), Estimated Support Volume: " << results[i].supportVolume << std::endl;
    }

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
