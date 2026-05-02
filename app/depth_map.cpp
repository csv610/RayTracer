#include "RayTracer.h"
#include "MeshIO.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "mesh_utils.h"

        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "depth.ppm";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    Scene scene;
    scene.addMesh(mesh);
    scene.commit();

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
        for (int x = 0; x < width; ++x) {
            Ray ray;
            ray.org = {center.x, center.y, center.z + diag * 1.5f};
            ray.dir = {(x / (float)width - 0.5f) * 1.2f, (0.5f - y / (float)height) * 1.2f, -1.0f};
            Hit hit = RayTracer::intersect(scene, ray);
            if (hit.hit) {
                depths[y * width + x] = hit.t;
                minDepth = std::min(minDepth, hit.t);
                maxDepth = std::max(maxDepth, hit.t);
            }
        }

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
