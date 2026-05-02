#include "CurvatureAnalyzer.h"
#include "MeshIO.h"
#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "curvature_output.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    try {
        CurvatureAnalyzer analyzer(mesh);
        auto res = analyzer.analyze();
        
        Mesh coloredMesh = res.getColoredMesh(mesh);
        MeshIO::save(outputFile, coloredMesh);
        
        printf("Curvature analysis complete. Saved to %s\n", outputFile.c_str());

    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    return 0;
}
