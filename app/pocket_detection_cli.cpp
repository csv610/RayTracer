#include "CADFeatureDetector.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("pocket_detection", 
        "Detect pockets and concave regions in a mesh that may collect debris or be hard to access.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: pockets.off)", "pockets.off");
    parser.add_argument("-n", "Number of ray samples per node (default: 64)", "64");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int samples = parser.get_int("n", 64);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    CADFeatureDetector detector(mesh);
    auto nodeExposure = detector.computePocketExposure(samples);

    mesh.faceColors.resize(mesh.triangles.size());
    for(size_t i = 0; i < mesh.triangles.size(); ++i) {
        const auto& tri = mesh.triangles[i];
        float avgExp = (nodeExposure[tri.v0] + nodeExposure[tri.v1] + nodeExposure[tri.v2]) / 3.0f;
        unsigned char c = (unsigned char)(avgExp * 255.0f);
        mesh.faceColors[i] = {c, c, c, 255};
    }
    
    MeshIO::save(outputFile, mesh);
    std::cout << "Pocket analysis complete. Saved to " << outputFile << std::endl;
    return 0;
}
