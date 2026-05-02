#include "RayTracer.h"
#include "MeshIO.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.ppm]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "depth.ppm";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    Scene scene;
    scene.addMesh(mesh);
    scene.commit();

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
    }

    for (int i = 0; i < width * height; ++i) {
        if (depths[i] >= 0) {
            float t = (maxDepth > minDepth) ? 1.0f - (depths[i] - minDepth) / (maxDepth - minDepth) : 1.0f;
            image[i] = {t, t, t};
        } else {
            image[i] = {0, 0, 0};
        }
    }

    MeshIO::savePPM(outputFile, width, height, image);
    std::cout << "Depth map saved to " << outputFile << std::endl;
    return 0;
}
