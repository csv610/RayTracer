#ifndef SHAPE_DIAMETER_H
#define SHAPE_DIAMETER_H

#include <embree4/rtcore.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include "mesh_utils.h"

class ShapeDiameter {
public:
    struct RayHit {
        int primID;
        Vec3 dir;
        float distance;
    };

    ShapeDiameter(const Mesh& mesh) : mesh(mesh) {
        device = rtcNewDevice(nullptr);
        if (device == nullptr) {
            throw std::runtime_error("Failed to create Embree device");
        }
        rtcSetDeviceErrorFunction(device, [](void* userPtr, RTCError code, const char* msg) {
            printf("Embree error %d: %s\n", code, msg);
        }, nullptr);
        scene = rtcNewScene(device);
        buildScene();
    }

    ~ShapeDiameter() {
        rtcReleaseScene(scene);
        rtcReleaseDevice(device);
    }

    void compute(int numTheta = 4, int numPhi = 8, float coneAngle = 0.5f) {
        shapeDiameters.assign(mesh.triangles.size(), 1e20f);
        
        printf("Computing shape diameter with %d inward rays per face...\n", numTheta * numPhi);
        fflush(stdout);

        for (size_t triIdx = 0; triIdx < mesh.triangles.size(); ++triIdx) {
            std::vector<RayHit> hits;
            computeForFace(triIdx, hits, numTheta, numPhi, coneAngle);
            
            if (!hits.empty()) {
                std::vector<float> distances;
                for (const auto& h : hits) distances.push_back(h.distance);
                std::sort(distances.begin(), distances.end());
                shapeDiameters[triIdx] = distances[distances.size() / 2];
            }
        }
    }

    void computeForFace(int triIdx, std::vector<RayHit>& hits, int numTheta = 4, int numPhi = 8, float coneAngle = 0.5f) const {
        hits.clear();
        if (triIdx < 0 || triIdx >= (int)mesh.triangles.size()) return;

        const Triangle& tri = mesh.triangles[triIdx];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        Vec3 inwardNormal = {-normal.x, -normal.y, -normal.z};
        Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        Vec3 up = (std::abs(inwardNormal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {inwardNormal.y * up.z - inwardNormal.z * up.y,
                        inwardNormal.z * up.x - inwardNormal.x * up.z,
                        inwardNormal.x * up.y - inwardNormal.y * up.x};
        float tLen = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
        if (tLen > 0) { tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen; }
        
        Vec3 bitangent = {inwardNormal.y * tangent.z - inwardNormal.z * tangent.y,
                          inwardNormal.z * tangent.x - inwardNormal.x * tangent.z,
                          inwardNormal.x * tangent.y - inwardNormal.y * tangent.x};

        for (int i = 0; i < numTheta; ++i) {
            for (int j = 0; j < numPhi; ++j) {
                float theta = coneAngle * (i + 0.5f) / numTheta; 
                float phi = 2.0f * M_PI * (j + 0.5f) / numPhi;
                float sinT = sin(theta); float cosT = cos(theta);
                float sinP = sin(phi);   float cosP = cos(phi);

                Vec3 rayDir = {
                    (tangent.x * cosP + bitangent.x * sinP) * sinT + inwardNormal.x * cosT,
                    (tangent.y * cosP + bitangent.y * sinP) * sinT + inwardNormal.y * cosT,
                    (tangent.z * cosP + bitangent.z * sinP) * sinT + inwardNormal.z * cosT
                };

                RTCRayHit rh;
                float epsilon = 0.0001f;
                rh.ray.org_x = faceCenter.x + inwardNormal.x * epsilon;
                rh.ray.org_y = faceCenter.y + inwardNormal.y * epsilon;
                rh.ray.org_z = faceCenter.z + inwardNormal.z * epsilon;
                rh.ray.dir_x = rayDir.x; rh.ray.dir_y = rayDir.y; rh.ray.dir_z = rayDir.z;
                rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = 0xFFFFFFFF;
                rh.ray.time = 0.0f; rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

                RTCIntersectArguments args;
                rtcInitIntersectArguments(&args);
                rtcIntersect1(scene, &rh, &args);

                if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                    hits.push_back({(int)rh.hit.primID, rayDir, rh.ray.tfar});
                }
            }
        }
    }

    const std::vector<float>& getDiameters() const { return shapeDiameters; }

    void getStats(float& minD, float& maxD, float& avgD) const {
        maxD = 0;
        minD = 1e20f;
        float sumDist = 0;
        int count = 0;
        for (float d : shapeDiameters) {
            if (d < 1e19f) {
                if (d > maxD) maxD = d;
                if (d < minD) minD = d;
                sumDist += d;
                count++;
            }
        }
        avgD = count > 0 ? sumDist / count : 0;
    }

private:
    const Mesh& mesh;
    std::vector<float> shapeDiameters;
    RTCDevice device;
    RTCScene scene;

    void buildScene() {
        RTCGeometry triangleMesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

        Vertex* vertBuffer = (Vertex*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
        for (size_t i = 0; i < mesh.vertices.size(); ++i) vertBuffer[i] = mesh.vertices[i];

        Triangle* triBuffer = (Triangle*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
        for (size_t i = 0; i < mesh.triangles.size(); ++i) triBuffer[i] = mesh.triangles[i];

        rtcSetGeometryBuildQuality(triangleMesh, RTC_BUILD_QUALITY_HIGH);
        rtcCommitGeometry(triangleMesh);
        rtcAttachGeometry(scene, triangleMesh);
        rtcReleaseGeometry(triangleMesh);
        rtcCommitScene(scene);
    }
};

#endif // SHAPE_DIAMETER_H
