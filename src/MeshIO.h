#ifndef MESH_IO_H
#define MESH_IO_H

#include "mesh_utils.h"
#include <string>
#include <vector>

class MeshIO {
public:
    static bool load(const std::string& filename, Mesh& mesh);
    static bool save(const std::string& filename, const Mesh& mesh);
    static bool savePPM(const std::string& filename, int width, int height, const std::vector<Vec3>& image);

private:
    static bool readOFF(const std::string& filename, Mesh& mesh);
    static bool readPLY(const std::string& filename, Mesh& mesh);
    static bool writePLY(const std::string& filename, const Mesh& mesh);
    static bool loadWithAssimp(const std::string& filename, Mesh& mesh);
};

#endif
