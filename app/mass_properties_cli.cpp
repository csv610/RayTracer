#include "PhysicalProperties.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <iomanip>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("mass_properties", 
        "Calculate volume, center of mass, and inertia tensor using ray integration.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-r", "Ray cast resolution (default: 128)", "128");
    parser.add_argument("-d", "Material density (default: 1.0)", "1.0");
    parser.add_argument("-o", "Output file (optional)", "");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    Mesh mesh;
    std::cerr << "CLI: Loading file: " << parser.get("input") << std::endl;
    if (!MeshIO::load(parser.get("input"), mesh)) {
        std::cerr << "Error: Failed to load mesh: " << parser.get("input") << std::endl;
        return 1;
    }
    std::cerr << "CLI: Loaded " << mesh.vertices.size() << " vertices and " << mesh.triangles.size() << " triangles" << std::endl;

    int res = parser.get_int("r", 128);
    float density = parser.get_float("d", 1.0f);

    PhysicalProperties analyzer(mesh);
    auto p = analyzer.compute(res);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\nMass Properties (density = " << density << "):" << std::endl;
    std::cout << "Volume: " << p.volume << std::endl;
    std::cout << "Mass: " << p.volume * density << std::endl;
    std::cout << "CoM:    (" << p.centerOfMass.x << ", " << p.centerOfMass.y << ", " << p.centerOfMass.z << ")" << std::endl;
    std::cout << "Inertia Tensor at CoM:" << std::endl;
    for(int i = 0; i < 3; ++i) {
        std::cout << "| " << std::setw(12) << p.inertiaTensor[i][0] 
                  << " " << std::setw(12) << p.inertiaTensor[i][1] 
                  << " " << std::setw(12) << p.inertiaTensor[i][2] << " |" << std::endl;
    }
    return 0;
}