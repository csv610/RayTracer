#include "AssemblyAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("assembly_clearance", 
        "Analyze clearance between two parts to detect collisions and violations.");

    parser.add_positional("part_a", "First part mesh file (OFF/PLY format)");
    parser.add_positional("part_b", "Second part mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: clearance_results.off)", "clearance_results.off");
    parser.add_argument("-t", "Clearance threshold distance (default: 1.0)", "1.0");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFileA = parser.get("part_a");
    std::string inputFileB = parser.get("part_b");
    std::string outputFile = parser.get("o");
    float threshold = parser.get_float("threshold", 1.0f);

    Mesh meshA, meshB;
    if (!MeshIO::load(inputFileA, meshA) || !MeshIO::load(inputFileB, meshB)) return 1;

    AssemblyAnalyzer analyzer(meshA, meshB);
    auto res = analyzer.analyzeClearance(threshold);

    MeshIO::save(outputFile, res.getColoredMesh(meshA));

    std::cout << "Analysis complete: " << res.collisions << " collisions, " << res.violations << " clearance violations." << std::endl;

    return 0;
}