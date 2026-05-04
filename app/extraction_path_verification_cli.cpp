#include "AssemblyAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>
#include <sstream>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("extraction_path_verification", 
        "Verify that a part can be extracted from an environment along a given direction.");

    parser.add_positional("part", "Part mesh file (OFF/PLY format)");
    parser.add_positional("environment", "Environment mesh file (OFF/PLY format)");
    parser.add_argument("-d", "Extraction direction as 'dx dy dz' (required)", "");
    parser.add_argument("-l", "Extraction distance (default: 100)", "100");
    parser.add_argument("-o", "Output mesh file (default: extraction.off)", "extraction.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string partFile = parser.get("part");
    std::string envFile = parser.get("environment");
    std::string outputFile = parser.get("o");
    float dist = parser.get_float("l", 100.0f);
    
    std::string dirStr = parser.get("d");
    if (dirStr.empty()) {
        std::cerr << "Error: --direction is required" << std::endl;
        return 1;
    }
    
    float dx = 0, dy = 0, dz = 1;
    std::istringstream iss(dirStr);
    iss >> dx >> dy >> dz;
    Vec3 d = {dx, dy, dz};

    Mesh part, env;
    if (!MeshIO::load(partFile, part) || !MeshIO::load(envFile, env)) return 1;

    AssemblyAnalyzer analyzer(part, env);
    auto res = analyzer.verifyExtractionPath(d, dist);
    MeshIO::save(outputFile, res.getColoredMesh(part));

    std::cout << "Detected " << res.collisions << " triangles with collisions along path." << std::endl;

    return 0;
}