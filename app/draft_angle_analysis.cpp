#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [pull_x pull_y pull_z] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    Vec3 pullDir = {0, 0, 1}; // Default +Z
    int outIdx = 2;
    if (argc >= 5) {
        pullDir = {(float)atof(argv[2]), (float)atof(argv[3]), (float)atof(argv[4])};
        float len = sqrt(pullDir.x*pullDir.x + pullDir.y*pullDir.y + pullDir.z*pullDir.z);
        if (len > 0) { pullDir.x /= len; pullDir.y /= len; pullDir.z /= len; }
        outIdx = 5;
    }
    std::string outputFile = (argc > outIdx) ? argv[outIdx] : "draft_angles.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    std::vector<Vec3> triColors(mesh.triangles.size());
    std::cout << "Analyzing draft angles for pull direction (" << pullDir.x << ", " << pullDir.y << ", " << pullDir.z << ")..." << std::endl;

    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        const Triangle& tri = mesh.triangles[i];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        float dot = normal.x * pullDir.x + normal.y * pullDir.y + normal.z * pullDir.z;
        dot = std::clamp(dot, -1.0f, 1.0f);
        
        // Draft angle is the angle relative to the perpendicular of the pull direction
        // 90 deg - acos(dot)
        float angleRad = M_PI * 0.5f - acos(dot);
        float angleDeg = angleRad * 180.0f / M_PI;

        Vec3 color;
        if (angleDeg < -0.1f) {
            color = {1.0f, 0.0f, 0.0f}; // Red: Undercut
        } else if (angleDeg < 3.0f) {
            // Near-vertical (0 to 3 deg) -> Yellow to Green transition
            float t = std::max(0.0f, angleDeg / 3.0f);
            color = {1.0f, t, 0.0f}; // Yellow/Orange
        } else {
            color = {0.0f, 1.0f, 0.0f}; // Green: Safe draft
        }
        triColors[i] = color;
    }

    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << mesh.vertices.size() << " " << mesh.triangles.size() << " 0" << std::endl;
    for (const auto& v : mesh.vertices) out << v.x << " " << v.y << " " << v.z << std::endl;
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        const auto& t = mesh.triangles[i];
        const auto& c = triColors[i];
        out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << " " << c.x << " " << c.y << " " << c.z << std::endl;
    }
    out.close();

    std::cout << "Draft angle analysis saved to " << outputFile << std::endl;
    return 0;
}
