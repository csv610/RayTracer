#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [pull_x pull_y pull_z]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "undercuts.off";
    
    Vec3 pullDir = {0, 0, 1}; // Default +Z
    if (argc >= 6) {
        pullDir = {(float)atof(argv[3]), (float)atof(argv[4]), (float)atof(argv[5])};
        float len = sqrt(pullDir.x*pullDir.x + pullDir.y*pullDir.y + pullDir.z*pullDir.z);
        if (len > 0) { pullDir.x /= len; pullDir.y /= len; pullDir.z /= len; }
    }

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
    float rayLength = radius * 3.0f;
    float epsilon = radius * 1e-4f;

    std::vector<Vec3> triColors(mesh.triangles.size());
    int undercutCount = 0;

    std::cout << "Analyzing undercuts for pull direction (" << pullDir.x << ", " << pullDir.y << ", " << pullDir.z << ")..." << std::endl;

    tbb::parallel_for(size_t(0), mesh.triangles.size(), [&](size_t i) {
        const Triangle& tri = mesh.triangles[i];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        float dot = normal.x * pullDir.x + normal.y * pullDir.y + normal.z * pullDir.z;
        bool isUndercut = false;

        if (dot < -0.01f) {
            // Points away from pull direction
            isUndercut = true;
        } else {
            // Points towards pull direction, check for occlusion
            RTCRayHit rh;
            rh.ray.org_x = faceCenter.x + normal.x * epsilon;
            rh.ray.org_y = faceCenter.y + normal.y * epsilon;
            rh.ray.org_z = faceCenter.z + normal.z * epsilon;
            rh.ray.dir_x = pullDir.x;
            rh.ray.dir_y = pullDir.y;
            rh.ray.dir_z = pullDir.z;
            rh.ray.tnear = 0.0f;
            rh.ray.tfar = rayLength;
            rh.ray.mask = -1;
            rh.ray.time = 0;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args;
            rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                isUndercut = true;
            }
        }

        if (isUndercut) {
            triColors[i] = {1.0f, 0.0f, 0.0f}; // Red
        } else {
            triColors[i] = {0.0f, 1.0f, 0.0f}; // Green
        }
    });

    // Save as OFF
    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << mesh.vertices.size() << " " << mesh.triangles.size() << " 0" << std::endl;
    for (const auto& v : mesh.vertices) out << v.x << " " << v.y << " " << v.z << std::endl;
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        const auto& t = mesh.triangles[i];
        const auto& c = triColors[i];
        out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << " " << c.x << " " << c.y << " " << c.z << std::endl;
    }

    for(const auto& c : triColors) if (c.x > 0.5f) undercutCount++;
    std::cout << "Detected " << undercutCount << " undercut triangles. Saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
