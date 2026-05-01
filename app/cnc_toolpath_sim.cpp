#include "SimulationSuite.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [res] [tool_r] [out.off]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int res = (argc >= 3) ? std::atoi(argv[2]) : 128;
    float toolR = (argc >= 4) ? (float)atof(argv[3]) : 2.0f;
    std::string outF = (argc >= 5) ? argv[4] : "cnc_sim.off";

    SimulationSuite suite(mesh);
    auto cres = suite.simulateCnc(res, toolR);
    MeshIO::save(outF, cres);

    for (const auto& t : cres.meshTriangles) out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << "\n";
    std::cout << "CNC simulation saved to " << outF << std::endl;
    return 0;
}
