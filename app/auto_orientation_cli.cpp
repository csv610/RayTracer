#include "AutoOrientationOptimizer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <algorithm>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("auto_orientation", 
        "Find optimal build orientation for additive manufacturing by analyzing support volume.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-n", "Number of candidate orientations to evaluate (default: 32)", "32");
    parser.add_argument("-t", "Number of top results to display (default: 5)", "5");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    int samples = parser.get_int("samples", 32);
    int topN = parser.get_int("top", 5);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    AutoOrientationOptimizer optimizer(mesh);
    auto results = optimizer.optimize(samples);

    std::cout << "\nOptimal Build Orientation Analysis:" << std::endl;
    for (int i = 0; i < std::min(topN, (int)results.size()); ++i) {
        std::cout << "Rank " << i+1 << ": UpDirection(" << results[i].up.x << ", " << results[i].up.y << ", " << results[i].up.z 
                  << "), Support Volume: " << results[i].supportVolume << std::endl;
    }

    return 0;
}