#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <fstream>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> <num_layers> [resolution]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int numLayers = std::atoi(argv[2]);
    int res = (argc >= 4) ? std::atoi(argv[3]) : 128;

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size();
    float layerThickness = size.z / numLayers;

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

    std::cout << "Slicing mesh into " << numLayers << " layers with resolution " << res << "x" << res << "..." << std::endl;

    for (int l = 0; l < numLayers; ++l) {
        float z = bbox.min.z + (l + 0.5f) * layerThickness;
        std::vector<uint8_t> slice(res * res, 0);

        tbb::parallel_for(0, res, [&](int y) {
            for (int x = 0; x < res; ++x) {
                float px = bbox.min.x + (x + 0.5f) * (size.x / res);
                float py = bbox.min.y + (y + 0.5f) * (size.y / res);

                RTCRayHit rh;
                rh.ray.org_x = px; rh.ray.org_y = py; rh.ray.org_z = z;
                rh.ray.dir_x = 0.314f; rh.ray.dir_y = 0.718f; rh.ray.dir_z = 0.941f;
                float len = sqrt(rh.ray.dir_x*rh.ray.dir_x + rh.ray.dir_y*rh.ray.dir_y + rh.ray.dir_z*rh.ray.dir_z);
                rh.ray.dir_x /= len; rh.ray.dir_y /= len; rh.ray.dir_z /= len;
                rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = -1;
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
                if (intersections % 2 != 0) slice[y * res + x] = 255;
            }
        });

        // Optionally save as PPM for a few layers to verify
        if (l % std::max(1, numLayers/5) == 0) {
            std::string filename = "slice_" + std::to_string(l) + ".ppm";
            std::ofstream out(filename, std::ios::binary);
            out << "P5\n" << res << " " << res << "\n255\n";
            out.write((char*)slice.data(), slice.size());
            out.close();
            std::cout << "Saved verification slice to " << filename << std::endl;
        }
    }

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
