#include "ShapeDiameter.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("shape_diameter", 
        "Compute shape diameter function (SDF) for each face of the mesh.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: shape_diameter_output.off)", "shape_diameter_output.off");
    parser.add_argument("-n", "Number of ray samples per face (default: 64)", "64");

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

    try {
        ShapeDiameter sd(mesh);
        sd.compute(samples);

        float minDist, maxDist, avgDist;
        sd.getStats(minDist, maxDist, avgDist);
        printf("Distance range: [%.4f, %.4f], avg: %.4f\n", minDist, maxDist, avgDist);

        const auto& shapeDiameters = sd.getDiameters();
        mesh.faceColors.resize(mesh.triangles.size());
        for (size_t i = 0; i < mesh.triangles.size(); ++i) {
            float d = shapeDiameters[i];
            float t = (maxDist > minDist) ? (d - minDist) / (maxDist - minDist) : 0.0f;
            mesh.faceColors[i] = (d > 1e19f) ? Color4b{0, 0, 0, 255} : getJetColor(t);
        }
        MeshIO::save(outputFile, mesh);
        printf("Output written to %s\n", outputFile.c_str());

    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    return 0;
}