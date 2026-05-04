#include "AssemblyAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("inter_part_visibility", 
        "Analyze visibility of a target part from different directions in an environment.");

    parser.add_positional("target", "Target mesh file (OFF/PLY format)");
    parser.add_positional("environment", "Environment mesh file (OFF/PLY format)");
    parser.add_argument("-d", "View direction as 'vx vy vz' (required)", "");
    parser.add_argument("-o", "Output mesh file (default: visibility.off)", "visibility.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string targetFile = parser.get("target");
    std::string envFile = parser.get("environment");
    std::string outputFile = parser.get("o");
    
    std::string dirStr = parser.get("d");
    if (dirStr.empty()) {
        std::cerr << "Error: --direction is required" << std::endl;
        return 1;
    }
    
    float vx = 0, vy = 0, vz = 1;
    std::istringstream iss(dirStr);
    iss >> vx >> vy >> vz;
    Vec3 v = {vx, vy, vz};

    Mesh target, env;
    if (!MeshIO::load(targetFile, target) || !MeshIO::load(envFile, env)) return 1;

    AssemblyAnalyzer analyzer(target, env);
    auto colors = analyzer.analyzeVisibility(v);
    target.faceColors = colors;
    MeshIO::save(outputFile, target);

    std::cout << "Visibility analysis saved to " << outputFile << std::endl;

    return 0;
}