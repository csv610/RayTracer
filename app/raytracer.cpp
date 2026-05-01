#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.ppm]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "render.ppm";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) {
        return 1;
    }

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

    const int width = 800;
    const int height = 600;
    std::vector<Vec3> image(width * height);

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 center = {(bbox.min.x + bbox.max.x) * 0.5f, (bbox.min.y + bbox.max.y) * 0.5f, (bbox.min.z + bbox.max.z) * 0.5f};
    float diag = (bbox.size()).length();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            RTCRayHit rh;
            rh.ray.org_x = center.x; rh.ray.org_y = center.y; rh.ray.org_z = center.z + diag;
            rh.ray.dir_x = (x / (float)width - 0.5f);
            rh.ray.dir_y = (0.5f - y / (float)height);
            rh.ray.dir_z = -1.0f;
            rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = -1;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args; rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                float norm = std::sqrt(rh.hit.Ng_x*rh.hit.Ng_x + rh.hit.Ng_y*rh.hit.Ng_y + rh.hit.Ng_z*rh.hit.Ng_z);
                image[y * width + x] = {std::abs(rh.hit.Ng_x/norm), std::abs(rh.hit.Ng_y/norm), std::abs(rh.hit.Ng_z/norm)};
            } else {
                image[y * width + x] = {0.2f, 0.2f, 0.2f};
            }
        }
    }

    std::ofstream out(outputFile, std::ios::binary);
    out << "P6\n" << width << " " << height << "\n255\n";
    for (const auto& c : image) {
        unsigned char r = (unsigned char)(std::clamp(c.x, 0.0f, 1.0f) * 255);
        unsigned char g = (unsigned char)(std::clamp(c.y, 0.0f, 1.0f) * 255);
        unsigned char b = (unsigned char)(std::clamp(c.z, 0.0f, 1.0f) * 255);
        out << r << g << b;
    }
    std::cout << "Saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
