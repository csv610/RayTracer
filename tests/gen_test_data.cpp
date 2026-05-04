#include "MeshIO.h"
#include "mesh_utils.h"
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    Mesh mesh;
    createUVSphere(mesh, 20, 20, 1.0f);
    
    // Always try to create dataset in the current directory
    std::filesystem::create_directories("dataset");
    
    // Also try to create it in the parent directory if we are in build/
    if (std::filesystem::exists("../CMakeLists.txt")) {
        std::filesystem::create_directories("../dataset");
        MeshIO::save("../dataset/ter.off", mesh);
    }

    if (MeshIO::save("dataset/ter.off", mesh)) {
        std::cout << "Successfully generated dataset/ter.off" << std::endl;
        return 0;
    } else {
        std::cerr << "Failed to generate dataset/ter.off" << std::endl;
        return 1;
    }
}
