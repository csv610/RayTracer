#include <embree4/rtcore.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <random>
#include "mesh_utils.h"

int main(int argc, char** argv) {
    int stacks = 32;
    int slices = 64;
    int numRays = 1000000;
    
    if (argc >= 2) numRays = atoi(argv[1]);
    if (argc >= 3) stacks = atoi(argv[2]);
    if (argc >= 4) slices = atoi(argv[3]);

    printf("Sphere Benchmark: %d stacks, %d slices, %d rays\n", stacks, slices, numRays);

    RTCDevice device = rtcNewDevice(nullptr);
    if (device == nullptr) {
        printf("Failed to create device\n");
        return 1;
    }
    RTCScene scene = rtcNewScene(device);

    RTCGeometry triangleMesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    Mesh mesh;
    createUVSphere(mesh, stacks, slices, 1.0f);

    Vertex* vertBuffer = (Vertex*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) vertBuffer[i] = mesh.vertices[i];

    Triangle* triBuffer = (Triangle*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    for (size_t i = 0; i < mesh.triangles.size(); ++i) triBuffer[i] = mesh.triangles[i];

    rtcCommitGeometry(triangleMesh);
    rtcAttachGeometry(scene, triangleMesh);
    rtcReleaseGeometry(triangleMesh);
    rtcCommitScene(scene);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distTheta(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> distPhi(0.0f, M_PI);
    std::uniform_real_distribution<float> distDist(2.0f, 10.0f);

    RTCRayHit* rays = new RTCRayHit[numRays];
    for (int i = 0; i < numRays; ++i) {
        float theta = distTheta(rng);
        float phi = distPhi(rng);
        float dist = distDist(rng);
        
        rays[i].ray.org_x = dist * sin(phi) * cos(theta);
        rays[i].ray.org_y = dist * sin(phi) * sin(theta);
        rays[i].ray.org_z = dist * cos(phi);
        
        float nx = rays[i].ray.org_x;
        float ny = rays[i].ray.org_y;
        float nz = rays[i].ray.org_z;
        float len = sqrt(nx*nx + ny*ny + nz*nz);
        rays[i].ray.dir_x = -nx / len;
        rays[i].ray.dir_y = -ny / len;
        rays[i].ray.dir_z = -nz / len;
        
        rays[i].ray.tnear = 0.0f;
        rays[i].ray.tfar = INFINITY;
        rays[i].ray.mask = 0xFFFFFFFF;
        rays[i].ray.time = 0.0f;
        rays[i].hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rays[i].hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
    }

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    args.flags = RTC_RAY_QUERY_FLAG_NONE;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numRays; ++i) {
        rtcIntersect1(scene, &rays[i], &args);
    }
    auto end = std::chrono::high_resolution_clock::now();

    int hitCount = 0;
    for (int i = 0; i < numRays; ++i) {
        if (rays[i].hit.geomID != RTC_INVALID_GEOMETRY_ID) hitCount++;
    }

    double elapsed = std::chrono::duration<double>(end - start).count();
    printf("Time: %.3f seconds\n", elapsed);
    printf("Rays/sec: %.0f\n", numRays / elapsed);
    printf("Hits: %d / %d (%.1f%%)\n", hitCount, numRays, 100.0 * hitCount / numRays);

    delete[] rays;
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);

    return 0;
}
