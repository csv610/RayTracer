#ifndef MESH_IO_H
#define MESH_IO_H

#include "Mesh.h"
#include <string>
#include <vector>

/**
 * @class MeshIO
 * @brief Static utility class for mesh input and output operations.
 * 
 * MeshIO provides a unified interface for loading and saving 3D meshes in 
 * multiple formats (PLY, OFF, etc.). It features custom optimized loaders 
 * for common formats and integrates the Assimp library for broader 
 * compatibility with standard CAD and 3D graphics files.
 */
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
