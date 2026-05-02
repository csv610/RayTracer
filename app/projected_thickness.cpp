#include "ThicknessAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "projected_thickness.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    ThicknessAnalyzer analyzer(mesh);
    auto res = analyzer.computeProjectedThickness();

    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);
    
    std::cout << "Thickness analysis saved to " << outputFile << std::endl;
    return 0;
}
