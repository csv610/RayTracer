#include "SymmetryDetector.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("symmetry_detection", 
        "Detect planar symmetry planes in a mesh using sampling and matching.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-n", "Number of sample points (default: 512)", "512");
    parser.add_argument("-t", "Symmetry tolerance distance (default: 0.01)", "0.01");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    int samples = parser.get_int("samples", 512);
    float tolerance = parser.get_float("tolerance", 0.01f);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    SymmetryDetector detector(mesh);
    auto res = detector.detectSymmetry();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\nCandidate Symmetry Planes:" << std::endl;
    for (size_t i = 0; i < res.candidates.size(); ++i) {
        const auto& c = res.candidates[i];
        std::cout << "Plane " << i << ": Normal(" << c.first.normal.x << ", " << c.first.normal.y << ", " << c.first.normal.z 
                  << "), Score: " << c.second << std::endl;
    }

    std::cout << "\nBest symmetry plane found:" << std::endl;
    std::cout << "Normal: (" << res.bestPlane.normal.x << ", " << res.bestPlane.normal.y << ", " << res.bestPlane.normal.z << ")" << std::endl;
    std::cout << "Score: " << res.bestScore << std::endl;

    if (res.quality == SymmetryDetector::Quality::STRONG) {
        std::cout << "Result: The mesh exhibits STRONG symmetry." << std::endl;
    } else if (res.quality == SymmetryDetector::Quality::MODERATE) {
        std::cout << "Result: The mesh exhibits MODERATE symmetry." << std::endl;
    } else {
        std::cout << "Result: No strong global symmetry detected." << std::endl;
    }

    return 0;
}