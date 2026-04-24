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
    std::string outputFile = (argc >= 3) ? argv[2] : "shape_diameter_output.off";

    printf("Loading mesh: %s\n", inputFile.c_str());
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    if (!readOFF(inputFile, vertices, triangles)) return 1;

    printf("Mesh: %zu vertices, %zu triangles\n", vertices.size(), triangles.size());

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

    Vertex* vertBuffer = (Vertex*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) vertBuffer[i] = vertices[i];

    Triangle* triBuffer = (Triangle*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), triangles.size());
    for (size_t i = 0; i < triangles.size(); ++i) triBuffer[i] = triangles[i];

    rtcSetGeometryBuildQuality(triangleMesh, RTC_BUILD_QUALITY_HIGH);
    rtcCommitGeometry(triangleMesh);
    rtcAttachGeometry(scene, triangleMesh);
    rtcReleaseGeometry(triangleMesh);
    rtcCommitScene(scene);

    std::vector<float> minDistances(triangles.size(), 1e20f);
    
    // Using a cone of rays around the inward normal
    const int NUM_THETA = 4;
    const int NUM_PHI = 8;
    const float CONE_ANGLE = 0.5f; // approx 30 degrees in radians

    printf("Computing shape diameter with %d inward rays per face...\n", NUM_THETA * NUM_PHI);
    fflush(stdout);

    for (size_t triIdx = 0; triIdx < triangles.size(); ++triIdx) {
        const Triangle& tri = triangles[triIdx];
        const Vertex& v0 = vertices[tri.v0];
        const Vertex& v1 = vertices[tri.v1];
        const Vertex& v2 = vertices[tri.v2];

        Vec3 normal = computeFaceNormal(v0, v1, v2);
        Vec3 inwardNormal = {-normal.x, -normal.y, -normal.z};
        Vec3 faceCenter = computeFaceCenter(v0, v1, v2);

        // Create coordinate frame for the inward normal
        Vec3 up = (std::abs(inwardNormal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {inwardNormal.y * up.z - inwardNormal.z * up.y,
                        inwardNormal.z * up.x - inwardNormal.x * up.z,
                        inwardNormal.x * up.y - inwardNormal.y * up.x};
        float tLen = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
        tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen;
        Vec3 bitangent = {inwardNormal.y * tangent.z - inwardNormal.z * tangent.y,
                          inwardNormal.z * tangent.x - inwardNormal.x * tangent.z,
                          inwardNormal.x * tangent.y - inwardNormal.y * tangent.x};

        float currentMin = 1e20f;

        for (int i = 0; i < NUM_THETA; ++i) {
            for (int j = 0; j < NUM_PHI; ++j) {
                float theta = CONE_ANGLE * (i + 0.5f) / NUM_THETA; 
                float phi = 2.0f * 3.14159f * (j + 0.5f) / NUM_PHI;

                float sinT = sin(theta);
                float cosT = cos(theta);
                float sinP = sin(phi);
                float cosP = cos(phi);

                Vec3 rayDir = {
                    (tangent.x * cosP + bitangent.x * sinP) * sinT + inwardNormal.x * cosT,
                    (tangent.y * cosP + bitangent.y * sinP) * sinT + inwardNormal.y * cosT,
                    (tangent.z * cosP + bitangent.z * sinP) * sinT + inwardNormal.z * cosT
                };

                RTCRayHit rh;
                float epsilon = 0.0001f;
                // Start slightly inside to avoid self-intersection with the originating triangle
                rh.ray.org_x = faceCenter.x + inwardNormal.x * epsilon;
                rh.ray.org_y = faceCenter.y + inwardNormal.y * epsilon;
                rh.ray.org_z = faceCenter.z + inwardNormal.z * epsilon;
                rh.ray.dir_x = rayDir.x;
                rh.ray.dir_y = rayDir.y;
                rh.ray.dir_z = rayDir.z;
                rh.ray.tnear = 0.0f;
                rh.ray.tfar = 1e10f;
                rh.ray.mask = 0xFFFFFFFF;
                rh.ray.time = 0.0f;
                rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

                RTCIntersectArguments args;
                rtcInitIntersectArguments(&args);
                rtcIntersect1(scene, &rh, &args);

                if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                    if (rh.ray.tfar < currentMin) {
                        currentMin = rh.ray.tfar;
                    }
                }
            }
        }
        minDistances[triIdx] = currentMin;
    }

    float maxDist = 0;
    float minDist = 1e20f;
    float sumDist = 0;
    int count = 0;
    for (float d : minDistances) {
        if (d < 1e19f) {
            if (d > maxDist) maxDist = d;
            if (d < minDist) minDist = d;
            sumDist += d;
            count++;
        }
    }
    float avgDist = count > 0 ? sumDist / count : 0;
    
    printf("Distance range: [%.4f, %.4f], avg: %.4f\n", minDist, maxDist, avgDist);

    FILE* out = fopen(outputFile.c_str(), "w");
    fprintf(out, "OFF\n");
    fprintf(out, "%zu %zu 0\n", vertices.size(), triangles.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        fprintf(out, "%.6f %.6f %.6f\n", vertices[i].x, vertices[i].y, vertices[i].z);
    }
    for (size_t i = 0; i < triangles.size(); ++i) {
        float d = minDistances[i];
        float t = 0;
        if (maxDist > minDist) {
            t = (d - minDist) / (maxDist - minDist);
        }
        Vec3 color = getJetColor(t);
        if (d > 1e19f) color = {0, 0, 0}; // Black for no intersection

        fprintf(out, "3 %d %d %d %.6f %.6f %.6f\n", 
                triangles[i].v0, triangles[i].v1, triangles[i].v2,
                color.x, color.y, color.z);
    }
    fclose(out);

    printf("Output written to %s\n", outputFile.c_str());

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);

    return 0;
}
