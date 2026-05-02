#include "RayTracer.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <random>
#include "mesh_utils.h"

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
    Scene scene;
    Mesh mesh;
    createUVSphere(mesh, stacks, slices, 1.0f);
    scene.addMesh(mesh);
    scene.commit();

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
    std::vector<Ray> rays(numRays);
    for (int i = 0; i < numRays; ++i) {
        float theta = distTheta(rng);
        float phi = distPhi(rng);
        float dist = distDist(rng);
        rays[i].org = {dist * sin(phi) * cos(theta), dist * sin(phi) * sin(theta), dist * cos(phi)};
        rays[i].dir = {-rays[i].org.x, -rays[i].org.y, -rays[i].org.z};
        float l = rays[i].dir.length();
        if(l>0) { rays[i].dir.x/=l; rays[i].dir.y/=l; rays[i].dir.z/=l; }
        rays[i].tnear = 0.0f;
        rays[i].tfar = 1e10f;
    }
    auto start = std::chrono::high_resolution_clock::now();
    int hitCount = 0;
    for (int i = 0; i < numRays; ++i) {
        Hit hit = RayTracer::intersect(scene, rays[i]);
        if (hit.hit) hitCount++;
    }
    auto end = std::chrono::high_resolution_clock::now();

