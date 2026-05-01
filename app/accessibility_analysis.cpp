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
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [tool_radius] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    float toolRadius = (argc >= 3) ? (float)atof(argv[2]) : 2.0f; // Default 2mm tool
    std::string outputFile = (argc >= 4) ? argv[3] : "accessibility.off";

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
    float meshRadius;
    computeBoundingSphere(mesh, center, meshRadius);
    float rayLength = meshRadius * 4.0f;
    float epsilon = meshRadius * 1e-4f;

    std::vector<Vec3> triColors(mesh.triangles.size());
    
    std::cout << "Analyzing 3-axis CNC accessibility with tool radius: " << toolRadius << "..." << std::endl;

    tbb::parallel_for(size_t(0), mesh.triangles.size(), [&](size_t i) {
        const Triangle& tri = mesh.triangles[i];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        // Simple check: if normal points down, it's definitely inaccessible from +Z
        if (normal.z < 0.05f) {
            triColors[i] = {1.0f, 0.0f, 0.0f}; // Red
            return;
        }

        // To simulate a cylindrical tool of radius R, we cast multiple rays in +Z
        // We'll use 1 center ray + 8 rays on the perimeter of the tool
        const int numPerimeterRays = 8;
        bool accessible = true;

        for (int j = -1; j < numPerimeterRays; ++j) {
            float dx = 0, dy = 0;
            if (j >= 0) {
                float angle = (2.0f * M_PI * j) / numPerimeterRays;
                dx = cos(angle) * toolRadius;
                dy = sin(angle) * toolRadius;
            }

            RTCRayHit rh;
            rh.ray.org_x = faceCenter.x + dx;
            rh.ray.org_y = faceCenter.y + dy;
            rh.ray.org_z = faceCenter.z + epsilon;
            rh.ray.dir_x = 0;
            rh.ray.dir_y = 0;
            rh.ray.dir_z = 1.0f; // Pull direction +Z
            rh.ray.tnear = 0.0f;
            rh.ray.tfar = rayLength;
            rh.ray.mask = -1;
            rh.ray.time = 0;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args;
            rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                accessible = false;
                break;
            }
        }

        if (accessible) {
            triColors[i] = {0.0f, 1.0f, 0.0f}; // Green
        } else {
            triColors[i] = {1.0f, 0.0f, 0.0f}; // Red
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

    int inaccessible = 0;
    for(const auto& c : triColors) if (c.x > 0.5f) inaccessible++;
    std::cout << "Analysis complete: " << inaccessible << " triangles are inaccessible." << std::endl;
    std::cout << "Results saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
