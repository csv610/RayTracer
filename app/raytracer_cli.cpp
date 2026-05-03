#include "RayTracer.h"
#include "Renderer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <vector>
#include "mesh_utils.h"

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("raytracer", "Render normal map of a mesh using ray tracing.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output PPM image file (default: render.ppm)", "render.ppm");
    parser.add_argument("-w", "Image width (default: 800)", "800");
    parser.add_argument("-H", "Image height (default: 600)", "600");
    parser.add_argument("-f", "Camera field of view in degrees (default: 60)", "60");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int width = parser.get_int("w", 800);
    int height = parser.get_int("H", 600);
    float fov = parser.get_float("f", 60.0f);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Error: Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

    Scene scene;
    scene.addMesh(mesh);
    scene.commit();

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 center = {(bbox.min.x + bbox.max.x) * 0.5f, (bbox.min.y + bbox.max.y) * 0.5f, (bbox.min.z + bbox.max.z) * 0.5f};
    float diag = (bbox.size()).length();

    Renderer::Camera cam;
    cam.pos = {center.x, center.y, center.z + diag};
    cam.target = center;
    cam.up = {0, 1, 0};
    cam.fov = fov;
    cam.width = width;
    cam.height = height;

    std::vector<Vec3> image = Renderer::renderNormalMap(scene, cam);

    MeshIO::savePPM(outputFile, cam.width, cam.height, image);
    std::cout << "Saved to " << outputFile << std::endl;

    return 0;
}