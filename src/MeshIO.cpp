#include "MeshIO.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

bool MeshIO::load(const std::string& filename, Mesh& mesh) {
    mesh.vertices.clear();
    mesh.triangles.clear();

    std::string ext = "";
    size_t dot = filename.find_last_of(".");
    if (dot != std::string::npos) ext = filename.substr(dot + 1);

    if (ext == "ply") {
        if (readPLY(filename, mesh)) return true;
        std::cerr << "Custom PLY loader failed, falling back to Assimp..." << std::endl;
    } else if (ext == "off") {
        if (readOFF(filename, mesh)) return true;
        std::cerr << "Custom OFF loader failed, falling back to Assimp..." << std::endl;
    }

    return loadWithAssimp(filename, mesh);
}

bool MeshIO::readOFF(const std::string& filename, Mesh& mesh) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::string header;
    file >> header;
    if (header != "OFF" && header != "off") return false;
    int nVerts, nTris, nEdges;
    if (!(file >> nVerts >> nTris >> nEdges)) return false;
    mesh.vertices.resize(nVerts);
    for (int i = 0; i < nVerts; ++i) {
        if (!(file >> mesh.vertices[i].x >> mesh.vertices[i].y >> mesh.vertices[i].z)) return false;
    }
    mesh.triangles.resize(nTris);
    for (int i = 0; i < nTris; ++i) {
        int n;
        if (!(file >> n >> mesh.triangles[i].v0 >> mesh.triangles[i].v1 >> mesh.triangles[i].v2)) return false;
    }
    return true;
}

bool MeshIO::readPLY(const std::string& filename, Mesh& mesh) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    std::string line;
    if (!(file >> line) || line != "ply") return false;
    std::getline(file, line);

    struct Property { std::string type; std::string name; int size; };
    struct Element {
        std::string name; int count; std::vector<Property> props;
        bool isVertex = false; bool isFace = false; bool isTristrip = false;
    };
    std::vector<Element> elements;
    bool binary = false;

    auto getTypeSize = [](const std::string& t) {
        if (t == "char" || t == "uchar" || t == "int8" || t == "uint8") return 1;
        if (t == "short" || t == "ushort" || t == "int16" || t == "uint16") return 2;
        if (t == "int" || t == "uint" || t == "float" || t == "int32" || t == "uint32" || t == "float32") return 4;
        if (t == "double" || t == "int64" || t == "uint64" || t == "float64") return 8;
        return 0;
    };

    while (std::getline(file, line)) {
        if (line.find("format binary_little_endian") != std::string::npos) binary = true;
        if (line.find("element") != std::string::npos) {
            std::stringstream ss(line); std::string tmp, name; int count;
            ss >> tmp >> name >> count;
            Element e; e.name = name; e.count = count;
            if (name == "vertex") e.isVertex = true;
            else if (name == "face") e.isFace = true;
            else if (name == "tristrips") e.isTristrip = true;
            elements.push_back(e);
        } else if (line.find("property") != std::string::npos && !elements.empty()) {
            std::stringstream ss(line); std::string tmp, type, name; ss >> tmp;
            if (line.find("list") != std::string::npos) {
                Property p; p.type = "list"; p.name = "list"; p.size = 0;
                elements.back().props.push_back(p);
            } else {
                ss >> type >> name;
                Property p; p.type = type; p.name = name; p.size = getTypeSize(type);
                elements.back().props.push_back(p);
            }
        }
        if (line == "end_header") break;
    }
    if (!binary) return false;

    for (const auto& e : elements) {
        if (e.isVertex) {
            mesh.vertices.resize(e.count);
            bool hasColor = false;
            for (const auto& p : e.props) if (p.name == "red" || p.name == "diffuse_red") hasColor = true;
            if (hasColor) mesh.vertexColors.resize(e.count);

            for (int i = 0; i < e.count; ++i) {
                for (const auto& p : e.props) {
                    if (p.name == "x") file.read((char*)&mesh.vertices[i].x, 4);
                    else if (p.name == "y") file.read((char*)&mesh.vertices[i].y, 4);
                    else if (p.name == "z") file.read((char*)&mesh.vertices[i].z, 4);
                    else if (p.name == "red" || p.name == "diffuse_red") {
                        if (p.size == 1) { unsigned char c; file.read((char*)&c, 1); mesh.vertexColors[i].x = c / 255.0f; }
                        else if (p.size == 4) { float c; file.read((char*)&c, 4); mesh.vertexColors[i].x = c; }
                    }
                    else if (p.name == "green" || p.name == "diffuse_green") {
                        if (p.size == 1) { unsigned char c; file.read((char*)&c, 1); mesh.vertexColors[i].y = c / 255.0f; }
                        else if (p.size == 4) { float c; file.read((char*)&c, 4); mesh.vertexColors[i].y = c; }
                    }
                    else if (p.name == "blue" || p.name == "diffuse_blue") {
                        if (p.size == 1) { unsigned char c; file.read((char*)&c, 1); mesh.vertexColors[i].z = c / 255.0f; }
                        else if (p.size == 4) { float c; file.read((char*)&c, 4); mesh.vertexColors[i].z = c; }
                    }
                    else file.seekg(p.size, std::ios::cur);
                }
            }
        } else if (e.isFace) {
            bool hasColor = false;
            for (const auto& p : e.props) if (p.name == "red") hasColor = true;
            
            for (int i = 0; i < e.count; ++i) {
                unsigned char n; file.read((char*)&n, 1);
                std::vector<unsigned int> indices(n);
                for (int j = 0; j < n; ++j) {
                    unsigned int idx; file.read((char*)&idx, 4);
                    indices[j] = idx;
                }
                for (int j = 1; j < n - 1; ++j) {
                    mesh.triangles.push_back({indices[0], indices[j], indices[j+1]});
                    if (hasColor) mesh.faceColors.push_back({0,0,0}); // Placeholder or read if present
                }
                // Reading face colors if present (simplified)
                if (hasColor) {
                    Vec3 fc = {0,0,0};
                    for (const auto& p : e.props) {
                        if (p.name == "red") { unsigned char c; file.read((char*)&c, 1); fc.x = c/255.0f; }
                        else if (p.name == "green") { unsigned char c; file.read((char*)&c, 1); fc.y = c/255.0f; }
                        else if (p.name == "blue") { unsigned char c; file.read((char*)&c, 1); fc.z = c/255.0f; }
                        else if (p.name != "list") file.seekg(p.size, std::ios::cur);
                    }
                    // Since we triangulate, we might need to duplicate face color
                    for (int j = 1; j < n - 1; ++j) mesh.faceColors.back() = fc;
                }
            }
        } else if (e.isTristrip) {
            for (int i = 0; i < e.count; ++i) {
                int numIndices; file.read((char*)&numIndices, 4);
                std::vector<int> indices(numIndices);
                for (int j = 0; j < numIndices; ++j) file.read((char*)&indices[j], 4);
                
                int stripCount = 0;
                for (int j = 0; j < numIndices - 2; ++j) {
                    int v0 = indices[j], v1 = indices[j+1], v2 = indices[j+2];
                    if (v0 == -1 || v1 == -1 || v2 == -1) {
                        stripCount = 0;
                        continue;
                    }
                    if (stripCount % 2 == 0) mesh.triangles.push_back({(unsigned int)v0, (unsigned int)v1, (unsigned int)v2});
                    else mesh.triangles.push_back({(unsigned int)v0, (unsigned int)v2, (unsigned int)v1});
                    stripCount++;
                }
            }
        } else {
            for (int i = 0; i < e.count; ++i) {
                for (const auto& p : e.props) {
                    if (p.type == "list") { int n; file.read((char*)&n, 4); file.seekg(n * 4, std::ios::cur); }
                    else file.seekg(p.size, std::ios::cur);
                }
            }
        }
    }
    return true;
}

bool MeshIO::writePLY(const std::string& filename, const Mesh& mesh) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;

    out << "ply\n";
    out << "format binary_little_endian 1.0\n";
    out << "element vertex " << mesh.vertices.size() << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    bool hasNormals = mesh.vertexNormals.size() == mesh.vertices.size();
    if (hasNormals) {
        out << "property float nx\n";
        out << "property float ny\n";
        out << "property float nz\n";
    }
    bool hasVertexColors = mesh.vertexColors.size() == mesh.vertices.size();
    if (hasVertexColors) {
        out << "property uchar red\n";
        out << "property uchar green\n";
        out << "property uchar blue\n";
    }
    out << "element face " << mesh.triangles.size() << "\n";
    out << "property list uchar uint vertex_indices\n";
    bool hasFaceColors = mesh.faceColors.size() == mesh.triangles.size();
    if (hasFaceColors) {
        out << "property uchar red\n";
        out << "property uchar green\n";
        out << "property uchar blue\n";
    }
    out << "end_header\n";

    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        out.write((char*)&mesh.vertices[i].x, 4);
        out.write((char*)&mesh.vertices[i].y, 4);
        out.write((char*)&mesh.vertices[i].z, 4);
        if (hasNormals) {
            out.write((char*)&mesh.vertexNormals[i].x, 4);
            out.write((char*)&mesh.vertexNormals[i].y, 4);
            out.write((char*)&mesh.vertexNormals[i].z, 4);
        }
        if (hasVertexColors) {
            unsigned char r = (unsigned char)(std::clamp(mesh.vertexColors[i].x, 0.0f, 1.0f) * 255);
            unsigned char g = (unsigned char)(std::clamp(mesh.vertexColors[i].y, 0.0f, 1.0f) * 255);
            unsigned char b = (unsigned char)(std::clamp(mesh.vertexColors[i].z, 0.0f, 1.0f) * 255);
            out.write((char*)&r, 1);
            out.write((char*)&g, 1);
            out.write((char*)&b, 1);
        }
    }

    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        unsigned char n = 3;
        out.write((char*)&n, 1);
        out.write((char*)&mesh.triangles[i].v0, 4);
        out.write((char*)&mesh.triangles[i].v1, 4);
        out.write((char*)&mesh.triangles[i].v2, 4);
        if (hasFaceColors) {
            unsigned char r = (unsigned char)(std::clamp(mesh.faceColors[i].x, 0.0f, 1.0f) * 255);
            unsigned char g = (unsigned char)(std::clamp(mesh.faceColors[i].y, 0.0f, 1.0f) * 255);
            unsigned char b = (unsigned char)(std::clamp(mesh.faceColors[i].z, 0.0f, 1.0f) * 255);
            out.write((char*)&r, 1);
            out.write((char*)&g, 1);
            out.write((char*)&b, 1);
        }
    }

    return true;
}

bool MeshIO::loadWithAssimp(const std::string& filename, Mesh& mesh) {
    Assimp::Importer importer;
    const aiScene* aiS = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
    if (!aiS || !aiS->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }
    for (unsigned int i = 0; i < aiS->mNumMeshes; ++i) {
        aiMesh* m = aiS->mMeshes[i];
        unsigned int offset = mesh.vertices.size();
        for (unsigned int j = 0; j < m->mNumVertices; ++j) {
            mesh.vertices.push_back({m->mVertices[j].x, m->mVertices[j].y, m->mVertices[j].z});
        }
        for (unsigned int j = 0; j < m->mNumFaces; ++j) {
            if (m->mFaces[j].mNumIndices == 3)
                mesh.triangles.push_back({m->mFaces[j].mIndices[0] + offset, m->mFaces[j].mIndices[1] + offset, m->mFaces[j].mIndices[2] + offset});
        }
    }
    return true;
}

bool MeshIO::save(const std::string& filename, const Mesh& mesh) {
    std::string ext = "";
    size_t dot = filename.find_last_of(".");
    if (dot != std::string::npos) ext = filename.substr(dot + 1);

    if (ext == "ply") {
        return writePLY(filename, mesh);
    }

    std::ofstream out(filename);
    if (!out) return false;

    bool hasNormals = mesh.vertexNormals.size() == mesh.vertices.size();
    bool hasVertexColors = mesh.vertexColors.size() == mesh.vertices.size();
    bool hasFaceColors = mesh.faceColors.size() == mesh.triangles.size();

    if (hasNormals && hasVertexColors) out << "CNOFF\n";
    else if (hasNormals) out << "NOFF\n";
    else if (hasVertexColors) out << "COFF\n";
    else out << "OFF\n";

    out << mesh.vertices.size() << " " << mesh.triangles.size() << " 0\n";
    
    // Vertices
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        out << mesh.vertices[i].x << " " << mesh.vertices[i].y << " " << mesh.vertices[i].z;
        if (hasNormals) {
            out << " " << mesh.vertexNormals[i].x << " " << mesh.vertexNormals[i].y << " " << mesh.vertexNormals[i].z;
        }
        if (hasVertexColors) {
            out << " " << mesh.vertexColors[i].x << " " << mesh.vertexColors[i].y << " " << mesh.vertexColors[i].z << " 1.0";
        }
        out << "\n";
    }

    // Faces
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        out << "3 " << mesh.triangles[i].v0 << " " << mesh.triangles[i].v1 << " " << mesh.triangles[i].v2;
        if (hasFaceColors) {
            out << " " << mesh.faceColors[i].x << " " << mesh.faceColors[i].y << " " << mesh.faceColors[i].z << " 1.0";
        }
        out << "\n";
    }
    return true;
}

bool MeshIO::savePPM(const std::string& filename, int width, int height, const std::vector<Vec3>& image) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << width << " " << height << "\n255\n";
    for (const auto& c : image) {
        unsigned char r = (unsigned char)(std::clamp(c.x, 0.0f, 1.0f) * 255);
        unsigned char g = (unsigned char)(std::clamp(c.y, 0.0f, 1.0f) * 255);
        unsigned char b = (unsigned char)(std::clamp(c.z, 0.0f, 1.0f) * 255);
        out << r << g << b;
    }
    return true;
}
