#include "RayTracer.h"
#include "Renderer.h"
#include "MeshIO.h"
#include <iostream>
#include <vector>
#include "mesh_utils.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.ppm]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "render.ppm";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

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
    cam.fov = 60.0f;
    cam.width = 800;
    cam.height = 600;

    std::vector<Vec3> image = Renderer::renderNormalMap(scene, cam);

    MeshIO::savePPM(outputFile, cam.width, cam.height, image);
    std::cout << "Saved to " << outputFile << std::endl;

    return 0;
}
