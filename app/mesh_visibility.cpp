#include <embree4/rtcore.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <fstream>
#include "mesh_utils.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "mesh_output.off";

    printf("Loading mesh: %s\n", inputFile.c_str());
    Mesh mesh;
    if (!readOFF(inputFile, mesh)) return 1;

    printf("Mesh: %zu vertices, %zu triangles\n", mesh.vertices.size(), mesh.triangles.size());

    Vertex center;
    float sphereRadius;
    computeBoundingSphere(mesh, center, sphereRadius);
    float outerRadius = sphereRadius * 1.1f;

    printf("Bounding sphere: center=(%.3f, %.3f, %.3f), radius=%.3f\n",
           center.x, center.y, center.z, sphereRadius);

    RTCDevice device = rtcNewDevice(nullptr);
    if (device == nullptr) {
        printf("Failed to create device\n");
        return 1;
    }
    rtcSetDeviceErrorFunction(device, [](void* userPtr, RTCError code, const char* msg) {
        printf("Embree error %d: %s\n", code, msg);
    }, nullptr);
    RTCScene scene = rtcNewScene(device);

    RTCGeometry triangleMesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    Vertex* vertBuffer = (Vertex*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) vertBuffer[i] = mesh.vertices[i];

    Triangle* triBuffer = (Triangle*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    for (size_t i = 0; i < mesh.triangles.size(); ++i) triBuffer[i] = mesh.triangles[i];

    rtcSetGeometryBuildQuality(triangleMesh, RTC_BUILD_QUALITY_HIGH);
    rtcSetGeometryMask(triangleMesh, 0xFFFFFFFF);
    rtcCommitGeometry(triangleMesh);
    rtcAttachGeometry(scene, triangleMesh);
    rtcReleaseGeometry(triangleMesh);
    rtcCommitScene(scene);

    std::vector<Vertex> triColors(mesh.triangles.size(), {1.0f, 0.0f, 0.0f});
    int visibleCount = 0;
    int rayHits = 0;

    RTCRayHit testRay;
    testRay.ray.org_x = -10.0f; testRay.ray.org_y = 0.0f; testRay.ray.org_z = 0.0f;
    testRay.ray.dir_x = 1.0f; testRay.ray.dir_y = 0.0f; testRay.ray.dir_z = 0.0f;
    testRay.ray.tnear = 0.001f;
    testRay.ray.tfar = 100.0f;
    testRay.ray.mask = 0xFFFFFFFF;
    testRay.ray.time = 0.0f;
    testRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    testRay.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments testArgs;
    rtcInitIntersectArguments(&testArgs);
    testArgs.flags = RTC_RAY_QUERY_FLAG_NONE;
    rtcIntersect1(scene, &testRay, &testArgs);
    rayHits++;
    printf("Test ray from (-10,0,0) to (10,0,0): geomID=%u primID=%u tfar=%f\n", 
           testRay.hit.geomID, testRay.hit.primID, testRay.ray.tfar);

    const int NUM_THETA = 8;
    const int NUM_PHI = 16;

    printf("Computing triangle visibility with 2D hemispherical sampling (%d rays per face)...\n", NUM_THETA * NUM_PHI);
    fflush(stdout);

    for (size_t triIdx = 0; triIdx < mesh.triangles.size(); ++triIdx) {
        const Triangle& tri = mesh.triangles[triIdx];
        const Vertex& v0 = mesh.vertices[tri.v0];
        const Vertex& v1 = mesh.vertices[tri.v1];
        const Vertex& v2 = mesh.vertices[tri.v2];

        Vec3 normal = computeFaceNormal(v0, v1, v2);
        Vec3 faceCenter = computeFaceCenter(v0, v1, v2);

        // Create coordinate frame
        Vec3 up = (std::abs(normal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {normal.y * up.z - normal.z * up.y,
                        normal.z * up.x - normal.x * up.z,
                        normal.x * up.y - normal.y * up.x};
        float tLen = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
        tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen;
        Vec3 bitangent = {normal.y * tangent.z - normal.z * tangent.y,
                          normal.z * tangent.x - normal.x * tangent.z,
                          normal.x * tangent.y - normal.y * tangent.x};

        bool isVisible = false;
        for (int i = 0; i < NUM_THETA; ++i) {
            for (int j = 0; j < NUM_PHI; ++j) {
                float theta = 3.14159f * 0.5f * (i + 0.5f) / NUM_THETA; // 0 to PI/2
                float phi = 2.0f * 3.14159f * (j + 0.5f) / NUM_PHI;    // 0 to 2PI

                float sinT = sin(theta);
                float cosT = cos(theta);
                float sinP = sin(phi);
                float cosP = cos(phi);

                Vec3 rayDir = {
                    (tangent.x * cosP + bitangent.x * sinP) * sinT + normal.x * cosT,
                    (tangent.y * cosP + bitangent.y * sinP) * sinT + normal.y * cosT,
                    (tangent.z * cosP + bitangent.z * sinP) * sinT + normal.z * cosT
                };

                RTCRayHit rh;
                float epsilon = 0.0001f;
                rh.ray.org_x = faceCenter.x + normal.x * epsilon;
                rh.ray.org_y = faceCenter.y + normal.y * epsilon;
                rh.ray.org_z = faceCenter.z + normal.z * epsilon;
                rh.ray.dir_x = rayDir.x;
                rh.ray.dir_y = rayDir.y;
                rh.ray.dir_z = rayDir.z;
                rh.ray.tnear = 0.0f;
                rh.ray.tfar = sphereRadius * 10.0f;
                rh.ray.mask = 0xFFFFFFFF;
                rh.ray.time = 0.0f;
                rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

                RTCIntersectArguments args;
                rtcInitIntersectArguments(&args);
                rtcIntersect1(scene, &rh, &args);
                rayHits++;

                if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
                    isVisible = true;
                    break;
                }
            }
            if (isVisible) break;
        }

        if (isVisible) {
            visibleCount++;
            triColors[triIdx] = {0.0f, 1.0f, 0.0f};
        }
    }

    FILE* out = fopen(outputFile.c_str(), "w");
    fprintf(out, "OFF\n");
    fprintf(out, "%zu %zu 0\n", mesh.vertices.size(), mesh.triangles.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        fprintf(out, "%.6f %.6f %.6f\n", mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
    }
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        fprintf(out, "3 %d %d %d %.6f %.6f %.6f\n", 
                mesh.triangles[i].v0, mesh.triangles[i].v1, mesh.triangles[i].v2,
                triColors[i].x, triColors[i].y, triColors[i].z);
    }
    fclose(out);

    std::string colorFile = outputFile.substr(0, outputFile.find(".off")) + "_colors.txt";
    FILE* col = fopen(colorFile.c_str(), "w");
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        fprintf(col, "%.6f %.6f %.6f\n", triColors[i].x, triColors[i].y, triColors[i].z);
    }
    fclose(col);

    printf("Ray hits: %d, Triangle visibility: %d visible (green), %zu occluded (red)\n", rayHits, visibleCount, mesh.triangles.size() - visibleCount);
    printf("Output: %s\n", outputFile.c_str());
    printf("Colors: %s\n", colorFile.c_str());

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);

    return 0;
}
