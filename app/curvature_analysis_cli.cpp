#include "CurvatureAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("curvature_analysis", 
        "Analyze surface curvature using normal ray shooting method.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: curvature_output.off)", "curvature_output.off");

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