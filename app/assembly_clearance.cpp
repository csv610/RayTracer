#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

bool checkInside(RTCScene scene, const Vec3& p) {
    RTCRayHit rh;
    rh.ray.org_x = p.x; rh.ray.org_y = p.y; rh.ray.org_z = p.z;
    rh.ray.dir_x = 0.314f; rh.ray.dir_y = 0.718f; rh.ray.dir_z = 0.941f; 
    float len = sqrt(rh.ray.dir_x*rh.ray.dir_x + rh.ray.dir_y*rh.ray.dir_y + rh.ray.dir_z*rh.ray.dir_z);
    rh.ray.dir_x /= len; rh.ray.dir_y /= len; rh.ray.dir_z /= len;
    
    rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = -1; rh.ray.time = 0;
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    RTCIntersectArguments args; rtcInitIntersectArguments(&args);
    
    int intersections = 0;
    while (true) {
        rtcIntersect1(scene, &rh, &args);
        if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
        intersections++;
        rh.ray.tnear = rh.ray.tfar + 1e-4f;
        rh.ray.tfar = 1e10f;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }
    return (intersections % 2 != 0);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <partA.off> <partB.off> [clearance_threshold] [outputA_colored.off]" << std::endl;
        return 1;
    }

    std::string fileA = argv[1];
    std::string fileB = argv[2];
    float threshold = (argc >= 4) ? (float)atof(argv[3]) : 1.0f;
    std::string outputFile = (argc >= 5) ? argv[4] : "clearance_results.off";

    Mesh meshA, meshB;
    if (!MeshIO::load(fileA, meshA)) return 1;
    if (!MeshIO::load(fileB, meshB)) return 1;

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene sceneB = rtcNewScene(device);
    
    // Add Mesh B to scene for testing against Mesh A
    RTCGeometry geomB = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vbB = (Vertex*)rtcSetNewGeometryBuffer(geomB, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), meshB.vertices.size());
    memcpy(vbB, meshB.vertices.data(), meshB.vertices.size() * sizeof(Vertex));
    Triangle* ibB = (Triangle*)rtcSetNewGeometryBuffer(geomB, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), meshB.triangles.size());
    memcpy(ibB, meshB.triangles.data(), meshB.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geomB);
    rtcAttachGeometry(sceneB, geomB);
    rtcReleaseGeometry(geomB);
    rtcCommitScene(sceneB);

    std::vector<Vec3> colorsA(meshA.triangles.size());
    std::cout << "Analyzing clearance and collisions between Part A and Part B (Threshold: " << threshold << ")..." << std::endl;

    tbb::parallel_for(size_t(0), meshA.triangles.size(), [&](size_t i) {
        const Triangle& tri = meshA.triangles[i];
        Vec3 normal = computeFaceNormal(meshA.vertices[tri.v0], meshA.vertices[tri.v1], meshA.vertices[tri.v2]);
        Vec3 faceCenter = computeFaceCenter(meshA.vertices[tri.v0], meshA.vertices[tri.v1], meshA.vertices[tri.v2]);

        // 1. Check if center of face A is inside Part B (Collision)
        if (checkInside(sceneB, faceCenter)) {
            colorsA[i] = {1.0f, 0.0f, 0.0f}; // Red (Collision)
            return;
        }

        // 2. Check clearance using ray casting along normal
        RTCRayHit rh;
        rh.ray.org_x = faceCenter.x; rh.ray.org_y = faceCenter.y; rh.ray.org_z = faceCenter.z;
        rh.ray.dir_x = normal.x; rh.ray.dir_y = normal.y; rh.ray.dir_z = normal.z;
        rh.ray.tnear = 0.0f; rh.ray.tfar = threshold; rh.ray.mask = -1; rh.ray.time = 0;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        RTCIntersectArguments args; rtcInitIntersectArguments(&args);
        rtcIntersect1(sceneB, &rh, &args);

        if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            colorsA[i] = {1.0f, 0.5f, 0.0f}; // Orange (Clearance Violation)
        } else {
            // Also check opposite direction just in case (though normally A points towards B or away)
            rh.ray.dir_x = -normal.x; rh.ray.dir_y = -normal.y; rh.ray.dir_z = -normal.z;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rtcIntersect1(sceneB, &rh, &args);
            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                colorsA[i] = {1.0f, 0.5f, 0.0f}; // Orange
            } else {
                colorsA[i] = {0.0f, 1.0f, 0.0f}; // Green (Safe)
            }
        }
    });

    // Save Mesh A with colors
    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << meshA.vertices.size() << " " << meshA.triangles.size() << " 0" << std::endl;
    for (const auto& v : meshA.vertices) out << v.x << " " << v.y << " " << v.z << std::endl;
    for (size_t i = 0; i < meshA.triangles.size(); ++i) {
        const auto& t = meshA.triangles[i];
        const auto& c = colorsA[i];
        out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << " " << c.x << " " << c.y << " " << c.z << std::endl;
    }

    int collisions = 0, violations = 0;
    for(const auto& c : colorsA) {
        if (c.y == 0.0f && c.x == 1.0f) collisions++;
        else if (c.y == 0.5f) violations++;
    }
    std::cout << "Analysis complete: " << collisions << " collisions (Red), " << violations << " clearance violations (Orange)." << std::endl;
    std::cout << "Results saved to " << outputFile << std::endl;

    rtcReleaseScene(sceneB);
    rtcReleaseDevice(device);
    return 0;
}
