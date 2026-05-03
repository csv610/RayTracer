#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>
#include <cmath>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("undercut_detection", 
        "Detect undercuts in a mesh relative to a pull direction (for mold casting analysis).");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output colored mesh file (default: undercuts.off)", "undercuts.off");
    parser.add_argument("-p", "Pull direction as 'x y z' (default: 0 0 1)", "0 0 1");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    std::string pullStr = parser.get("pull-dir");

    float px = 0, py = 0, pz = 1;
    std::istringstream iss(pullStr);
    iss >> px >> py >> pz;
    
    Vec3 pullDir = {px, py, pz};
    float l = pullDir.length();
    if(l > 0) { pullDir.x /= l; pullDir.y /= l; pullDir.z /= l; }

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    ManufacturingAnalyzer analyzer(mesh);
    auto res = analyzer.analyzeUndercuts(pullDir);

    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);

    std::cout << "Detected " << res.count << " undercut triangles. Saved to " << outputFile << std::endl;

    return 0;
}