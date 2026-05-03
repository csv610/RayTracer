#include "VisibilityAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <cstdio>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("mesh_visibility", 
        "Compute per-face visibility from a default up direction.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: mesh_output.off)", "mesh_output.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    VisibilityAnalyzer analyzer(mesh);
    auto res = analyzer.computeVisibility();

    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);

    printf("Triangle visibility: %d visible (green), %zu occluded (red)\n", res.visibleCount, mesh.triangles.size() - res.visibleCount);
    return 0;
}