#include "SimulationSuite.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [res] [out.off]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int res = (argc >= 3) ? std::stoi(argv[2]) : 64;
    std::string outF = (argc >= 4) ? argv[3] : "voxels.off";

    SimulationSuite suite(mesh);
    auto vres = suite.voxelize(res);

    std::ofstream out(outF);
    out << "OFF\n" << vres.vertices.size() << " " << vres.triangles.size() << " 0\n";
    for (const auto& v : vres.vertices) out << v.x << " " << v.y << " " << v.z << "\n";
    for (const auto& t : vres.triangles) out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << "\n";
    std::cout << "Voxelization saved to " << outF << std::endl;
    return 0;
}
