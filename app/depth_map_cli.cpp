#include "RayTracer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("depth_map", 
        "Render depth map of mesh from a fixed camera position.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output PPM image file (default: depth.ppm)", "depth.ppm");
    parser.add_argument("-w", "Image width (default: 800)", "800");
    parser.add_argument("-h", "Image height (default: 600)", "600");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int width = parser.get_int("width", 800);
    int height = parser.get_int("height", 600);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

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