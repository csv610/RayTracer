#include "ShapeDiameter.h"
#include "MeshIO.h"
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "shape_diameter_output.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    try {
        ShapeDiameter sd(mesh);
        sd.compute();

        float minDist, maxDist, avgDist;
        sd.getStats(minDist, maxDist, avgDist);
        printf("Distance range: [%.4f, %.4f], avg: %.4f\n", minDist, maxDist, avgDist);

        const auto& shapeDiameters = sd.getDiameters();
        mesh.faceColors.resize(mesh.triangles.size());
        for (size_t i = 0; i < mesh.triangles.size(); ++i) {
            float d = shapeDiameters[i];
            float t = (maxDist > minDist) ? (d - minDist) / (maxDist - minDist) : 0.0f;
            mesh.faceColors[i] = (d > 1e19f) ? Vec3{0, 0, 0} : getJetColor(t);
        }
        MeshIO::save(outputFile, mesh);
        printf("Output written to %s\n", outputFile.c_str());

    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    return 0;
}
