#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [threshold_angle_deg]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "overhangs.off";
    float thresholdDeg = (argc >= 4) ? (float)atof(argv[3]) : 45.0f;
    
    // Convert threshold to angle relative to UP (+Z)
    // A surface at 45 deg to vertical has a normal at 135 deg to +Z
    float criticalCos = cos((180.0f - thresholdDeg) * M_PI / 180.0f);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    std::vector<Vec3> triColors(mesh.triangles.size());
    int overhangCount = 0;

    std::cout << "Analyzing overhangs with threshold " << thresholdDeg << " degrees..." << std::endl;

    tbb::parallel_for(size_t(0), mesh.triangles.size(), [&](size_t i) {
        const Triangle& tri = mesh.triangles[i];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        // Gravity Up is +Z
        float dot = normal.z; 

        if (dot < criticalCos) {
            // Overhang (points down too much)
            // Map color from Red (most extreme) to Yellow (at threshold)
            float t = (dot - (-1.0f)) / (criticalCos - (-1.0f));
            t = std::clamp(t, 0.0f, 1.0f);
            triColors[i] = {1.0f, t, 0.0f}; 
        } else {
            // Safe
            triColors[i] = {0.0f, 1.0f, 0.0f}; 
        }
    });

    // Save as OFF
    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << mesh.vertices.size() << " " << mesh.triangles.size() << " 0" << std::endl;
    for (const auto& v : mesh.vertices) out << v.x << " " << v.y << " " << v.z << std::endl;
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        const auto& t = mesh.triangles[i];
        const auto& c = triColors[i];
        out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << " " << c.x << " " << c.y << " " << c.z << std::endl;
    }

    for(const auto& c : triColors) if (c.x > 0.5f) overhangCount++;
    std::cout << "Detected " << overhangCount << " overhang triangles. Saved to " << outputFile << std::endl;

    return 0;
}
