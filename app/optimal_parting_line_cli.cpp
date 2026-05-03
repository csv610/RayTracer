#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("optimal_parting_line", 
        "Find optimal parting direction for injection molding by analyzing undercut distribution.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-n", "Number of sampling directions (default: 64)", "64");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    int samples = parser.get_int("samples", 64);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    ManufacturingAnalyzer analyzer(mesh);
    Vec3 best = analyzer.findOptimalPartingLine(samples);

    std::cout << "Optimal Pull Direction: (" << best.x << ", " << best.y << ", " << best.z << ")" << std::endl;

    return 0;
}