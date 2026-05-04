#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("overhang_analysis", 
        "Detect overhang areas in a mesh that require support structures in additive manufacturing.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output colored mesh file (default: overhangs.off)", "overhangs.off");
    parser.add_argument("-t", "Overhang threshold angle in degrees (default: 45)", "45");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    float threshold = parser.get_float("threshold", 45.0f);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    ManufacturingAnalyzer analyzer(mesh);
    auto res = analyzer.analyzeOverhangs(threshold);

    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);

    std::cout << "Detected " << res.count << " overhang triangles. Saved to " << outputFile << std::endl;

    return 0;
}
