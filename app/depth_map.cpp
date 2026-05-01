#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.ppm]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "depth.ppm";

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

    const int width = 800, height = 600;
    std::vector<Vec3> image(width * height);
    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 center = {(bbox.min.x + bbox.max.x) * 0.5f, (bbox.min.y + bbox.max.y) * 0.5f, (bbox.min.z + bbox.max.z) * 0.5f};
    float diag = bbox.size().length();

    float minDepth = 1e10f, maxDepth = -1e10f;
    std::vector<float> depths(width * height, -1.0f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            RTCRayHit rh;
            rh.ray.org_x = center.x; rh.ray.org_y = center.y; rh.ray.org_z = center.z + diag * 1.5f;
            rh.ray.dir_x = (x / (float)width - 0.5f) * 1.2f;
            rh.ray.dir_y = (0.5f - y / (float)height) * 1.2f;
            rh.ray.dir_z = -1.0f;
            rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = -1;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            RTCIntersectArguments args; rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                depths[y * width + x] = rh.ray.tfar;
                minDepth = std::min(minDepth, rh.ray.tfar);
                maxDepth = std::max(maxDepth, rh.ray.tfar);
            }
        }
    }

    for (int i = 0; i < width * height; ++i) {
        if (depths[i] >= 0) {
            float t = 1.0f - (depths[i] - minDepth) / (maxDepth - minDepth);
            image[i] = {t, t, t};
        } else {
            image[i] = {0, 0, 0};
        }
    }

    MeshIO::savePPM(outputFile, width, height, image);
    std::cout << "Depth map saved to " << outputFile << std::endl;

    rtcReleaseScene(scene); rtcReleaseDevice(device);
    return 0;
}
