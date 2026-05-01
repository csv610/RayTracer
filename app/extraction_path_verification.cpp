#include "AssemblyAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <part.off> <env.off> <dx dy dz> [dist] [out.off]" << std::endl;
        return 1;
    }

    Mesh part, env;
    if (!MeshIO::load(argv[1], part) || !MeshIO::load(argv[2], env)) return 1;

    Vec3 d = {(float)atof(argv[3]), (float)atof(argv[4]), (float)atof(argv[5])};
    float dist = (argc >= 7) ? (float)atof(argv[6]) : 100.0f;
    std::string outF = (argc >= 8) ? argv[7] : "extraction.off";

    AssemblyAnalyzer analyzer(part, env);
    auto res = analyzer.verifyExtractionPath(d, dist);

    std::ofstream out(outF);
    out << "OFF\n" << part.vertices.size() << " " << part.triangles.size() << " 0\n";
    for (const auto& v : part.vertices) out << v.x << " " << v.y << " " << v.z << "\n";
    for (size_t i = 0; i < part.triangles.size(); ++i) {
        out << "3 " << part.triangles[i].v0 << " " << part.triangles[i].v1 << " " << part.triangles[i].v2 
            << " " << res.colors[i].x << " " << res.colors[i].y << " " << res.colors[i].z << "\n";
    }
    std::cout << "Detected " << res.collisions << " triangles with collisions along path." << std::endl;

    return 0;
}
