#ifndef MESH_IO_H
#define MESH_IO_H

#include "mesh_utils.h"
#include <string>

class MeshIO {
public:
    static bool load(const std::string& filename, Mesh& mesh);

private:
    static bool readOFF(const std::string& filename, Mesh& mesh);
    static bool readPLY(const std::string& filename, Mesh& mesh);
    static bool loadWithAssimp(const std::string& filename, Mesh& mesh);
};

#endif
