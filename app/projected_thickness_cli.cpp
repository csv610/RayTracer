#include "ThicknessAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("projected_thickness", 
        "Compute minimum wall thickness using ray casting in multiple directions.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: projected_thickness.off)", "projected_thickness.off");
    parser.add_argument("-n", "Number of ray directions (default: 256)", "256");

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

    ThicknessAnalyzer analyzer(mesh);
    auto res = analyzer.computeProjectedThickness();

    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);
    
    std::cout << "Thickness analysis saved to " << outputFile << std::endl;
    return 0;
}