#include "GeometryAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [samples]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int samples = (argc >= 4) ? std::atoi(argv[3]) : 64;
    GeometryAnalyzer analyzer(mesh);
    auto exposure = analyzer.computePocketExposure(samples);

    std::string outF = (argc >= 3) ? argv[2] : "pockets.off";
    std::ofstream out(outF);
    out << "OFF\n" << mesh.vertices.size() << " " << mesh.triangles.size() << " 0\n";
    for (const auto& v : mesh.vertices) out << v.x << " " << v.y << " " << v.z << "\n";
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        // Red for low exposure (pockets), white for high
        out << "3 " << mesh.triangles[i].v0 << " " << mesh.triangles[i].v1 << " " << mesh.triangles[i].v2 
            << " 1 " << exposure[i] << " " << exposure[i] << "\n";
    }
    std::cout << "Pocket analysis complete. Saved to " << outF << std::endl;
    return 0;
}
