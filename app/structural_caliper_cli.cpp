#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include "MeshIO.h"
#include "StructuralCaliper.h"
#include "argparse/argparse.h"
#include "mesh_utils.h"

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("structural_caliper", 
        "Analyze wall thickness of a mesh using ray casting from random directions.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: thickness_analysis.off)", "thickness_analysis.off");
    parser.add_argument("-t", "Minimum thickness threshold (default: 1%% of bounding box diagonal)", "");
    parser.add_argument("-n", "Number of ray samples (default: 100000)", "100000");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int numSamples = parser.get_int("n", 100000);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    float threshold = -1.0f;
    std::string threshStr = parser.get("t");
    if (!threshStr.empty()) {
        threshold = std::stof(threshStr);
    } else {
        AABB box;
        for (const auto& v : mesh.vertices) box.expand(v);
        Vec3 s = box.size();
        float diag = s.length();
        threshold = diag * 0.01f;
        std::cout << "Using default threshold: " << threshold << std::endl;
    }

    try {
        StructuralCaliper caliper(mesh);
        auto results = caliper.analyze(numSamples, threshold);

        Mesh outMesh;
        for (const auto& res : results) {
            outMesh.vertices.push_back({res.p.x, res.p.y, res.p.z});
            outMesh.vertexColors.push_back(res.color);
        }
        MeshIO::save(outputFile, outMesh);

        std::cout << "Successfully saved structural analysis to: " << outputFile << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}