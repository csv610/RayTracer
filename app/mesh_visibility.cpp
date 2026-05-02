#include "VisibilityAnalyzer.h"
#include "MeshIO.h"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "mesh_output.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    VisibilityAnalyzer analyzer(mesh);
    auto res = analyzer.computeVisibility();

    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);

    printf("Triangle visibility: %d visible (green), %zu occluded (red)\n", res.visibleCount, mesh.triangles.size() - res.visibleCount);
    return 0;
}
