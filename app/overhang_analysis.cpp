#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [threshold_deg]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    float threshold = (argc >= 4) ? (float)atof(argv[3]) : 45.0f;

    ManufacturingAnalyzer analyzer(mesh);
    auto res = analyzer.analyzeOverhangs(threshold);

    std::string outputFile = (argc >= 3) ? argv[2] : "overhangs.off";
    std::ofstream out(outputFile);
    out << "OFF\n" << mesh.vertices.size() << " " << mesh.triangles.size() << " 0\n";
    for (const auto& v : mesh.vertices) out << v.x << " " << v.y << " " << v.z << "\n";
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        out << "3 " << mesh.triangles[i].v0 << " " << mesh.triangles[i].v1 << " " << mesh.triangles[i].v2 
            << " " << res.colors[i].x << " " << res.colors[i].y << " " << res.colors[i].z << "\n";
    }
    std::cout << "Detected " << res.count << " overhang triangles. Saved to " << outputFile << std::endl;

    return 0;
}
