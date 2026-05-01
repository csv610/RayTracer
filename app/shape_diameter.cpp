#include <embree4/rtcore.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <fstream>
#include "mesh_utils.h"
#include "MeshIO.h"
#include "ShapeDiameter.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "shape_diameter_output.off";

    printf("Loading mesh: %s\n", inputFile.c_str());
    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    printf("Mesh: %zu vertices, %zu triangles\n", mesh.vertices.size(), mesh.triangles.size());

    try {
        ShapeDiameter sd(mesh);
        sd.compute();

        float minDist, maxDist, avgDist;
        sd.getStats(minDist, maxDist, avgDist);
        printf("Distance range: [%.4f, %.4f], avg: %.4f\n", minDist, maxDist, avgDist);

        const auto& shapeDiameters = sd.getDiameters();

        FILE* out = fopen(outputFile.c_str(), "w");
        fprintf(out, "OFF\n");
        fprintf(out, "%zu %zu 0\n", mesh.vertices.size(), mesh.triangles.size());
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            fprintf(out, "%.6f %.6f %.6f\n", mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
        }
        for (size_t i = 0; i < mesh.triangles.size(); ++i) {
            float d = shapeDiameters[i];
            float t = 0;
            if (maxDist > minDist) {
                t = (d - minDist) / (maxDist - minDist);
            }
            Vec3 color = getJetColor(t);
            if (d > 1e19f) color = {0, 0, 0}; // Black for no intersection

            fprintf(out, "3 %d %d %d %.6f %.6f %.6f\n", 
                    mesh.triangles[i].v0, mesh.triangles[i].v1, mesh.triangles[i].v2,
                    color.x, color.y, color.z);
        }
        fclose(out);
        printf("Output written to %s\n", outputFile.c_str());

    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }

    return 0;
}
