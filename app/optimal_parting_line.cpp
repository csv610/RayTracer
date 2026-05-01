#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

struct PullDirectionResult {
    Vec3 dir;
    float score; // Percentage of triangles that are undercuts (lower is better)
};

float calculateUndercutScore(RTCScene scene, const Mesh& mesh, const Vec3& pullDir, float rayLength, float epsilon) {
    int undercutCount = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, mesh.triangles.size()),
        0,
        [&](const tbb::blocked_range<size_t>& r, int init) -> int {
            int localCount = init;
            for (size_t i = r.begin(); i != r.end(); ++i) {
                const Triangle& tri = mesh.triangles[i];
                Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
                Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

                float dot = normal.x * pullDir.x + normal.y * pullDir.y + normal.z * pullDir.z;
                bool isUndercut = false;

                if (dot < -0.01f) {
                    isUndercut = true;
                } else {
                    RTCRayHit rh;
                    rh.ray.org_x = faceCenter.x + normal.x * epsilon;
                    rh.ray.org_y = faceCenter.y + normal.y * epsilon;
                    rh.ray.org_z = faceCenter.z + normal.z * epsilon;
                    rh.ray.dir_x = pullDir.x; rh.ray.dir_y = pullDir.y; rh.ray.dir_z = pullDir.z;
                    rh.ray.tnear = 0.0f; rh.ray.tfar = rayLength; rh.ray.mask = -1;
                    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    RTCIntersectArguments args; rtcInitIntersectArguments(&args);
                    rtcIntersect1(scene, &rh, &args);
                    if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) isUndercut = true;
                }
                if (isUndercut) localCount++;
            }
            return localCount;
        },
        std::plus<int>()
    );
    return (float)undercutCount / mesh.triangles.size();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [num_search_directions]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int numSearchDirs = (argc >= 3) ? std::atoi(argv[2]) : 64;

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

    Vertex center;
    float radius;
    computeBoundingSphere(mesh, center, radius);
    float rayLength = radius * 4.0f;
    float epsilon = radius * 1e-4f;

    std::cout << "Searching for optimal pull direction using " << numSearchDirs << " samples..." << std::endl;

    std::vector<PullDirectionResult> results;
    for (int s = 0; s < numSearchDirs; ++s) {
        // Fibonacci sphere sampling for uniform directions
        float phi = acos(1.0f - 2.0f * (s + 0.5f) / numSearchDirs);
        float theta = M_PI * (1.0f + sqrt(5.0f)) * (s + 0.5f);
        
        Vec3 dir = {sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)};
        float score = calculateUndercutScore(scene, mesh, dir, rayLength, epsilon);
        results.push_back({dir, score});
        
        if (s % 10 == 0) std::cout << "Progress: " << (s*100/numSearchDirs) << "%" << std::endl;
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.score < b.score;
    });

    std::cout << "\nOptimal Parting Line Analysis:" << std::endl;
    std::cout << "------------------------------" << std::endl;
    for (int i = 0; i < std::min(5, (int)results.size()); ++i) {
        std::cout << "Rank " << i+1 << ": Direction(" << results[i].dir.x << ", " << results[i].dir.y << ", " << results[i].dir.z 
                  << "), Undercut Score: " << (results[i].score * 100.0f) << "%" << std::endl;
    }

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
