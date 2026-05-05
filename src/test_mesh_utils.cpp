#include "Mesh.h"
#include "MeshIO.h"
#include "MeshGeometry.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <sstream>

void test_computeFaceNormal() {
    Node v0 = {0, 0, 0};
    Node v1 = {1, 0, 0};
    Node v2 = {0, 1, 0};
    Vec3 n = MeshGeometry::computeFaceNormal(v0, v1, v2);
    assert(std::abs(n.x) < 1e-6);
    assert(std::abs(n.y) < 1e-6);
    assert(std::abs(n.z - 1.0f) < 1e-6);
    std::cout << "test_computeFaceNormal passed!" << std::endl;
}

void test_computeFaceCenter() {
    Node v0 = {0, 0, 0};
    Node v1 = {3, 0, 0};
    Node v2 = {0, 3, 0};
    Vec3 c = MeshGeometry::computeFaceCenter(v0, v1, v2);
    assert(std::abs(c.x - 1.0f) < 1e-6);
    assert(std::abs(c.y - 1.0f) < 1e-6);
    assert(std::abs(c.z) < 1e-6);
    std::cout << "test_computeFaceCenter passed!" << std::endl;
}

void test_computeBoundingSphere() {
    Mesh mesh;
    mesh.nodes = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}
    };
    Node center;
    float radius;
    MeshGeometry geom(mesh);
    geom.computeBoundingSphere(center, radius);
    assert(std::abs(center.x) < 1e-6);
    assert(std::abs(center.y) < 1e-6);
    assert(std::abs(center.z) < 1e-6);
    assert(std::abs(radius - 1.0f) < 1e-6);
    std::cout << "test_computeBoundingSphere passed!" << std::endl;
}

void test_createUVSphere() {
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 10, 10, 1.0f);
    assert(!mesh.nodes.empty());
    assert(!mesh.triangles.empty());
    std::cout << "test_createUVSphere passed!" << std::endl;
}

void test_readOFF() {
    const char* filename = "test_temp.off";
    std::ofstream file(filename);
    file << "OFF\n3 1 0\n0 0 0\n1 0 0\n0 1 0\n3 0 1 2\n";
    file.close();

    Mesh mesh;
    bool success = MeshIO::load(filename, mesh);
    assert(success);
    assert(mesh.nodes.size() == 3);
    assert(mesh.triangles.size() == 1);
    assert(mesh.triangles[0].v0 == 0);
    assert(mesh.triangles[0].v1 == 1);
    assert(mesh.triangles[0].v2 == 2);
    
    std::remove(filename);
    std::cout << "test_readOFF passed!" << std::endl;
}

void test_getJetColor() {
    Color4b c0 = MeshGeometry::getJetColor(0.0f);
    assert(c0.r == 0);
    assert(c0.g == 0);
    assert(c0.b == 128); // Jet at 0 is blue-ish

    Color4b c1 = MeshGeometry::getJetColor(1.0f);
    assert(c1.r == 128); // Jet at 1 is red-ish
    assert(c1.g == 0);
    assert(c1.b == 0);
    
    std::cout << "test_getJetColor passed!" << std::endl;
}

void test_mesh_colors() {
    Mesh mesh;
    mesh.nodes = {{0,0,0}, {1,0,0}, {0,1,0}};
    mesh.triangles = {{0,1,2}};
    mesh.nodeColors = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    mesh.faceColors = {{255, 255, 255, 255}};

    const std::string filename = "test_colors.ply";
    bool success = MeshIO::save(filename, mesh);
    assert(success);

    Mesh loaded;
    success = MeshIO::load(filename, loaded);
    assert(success);
    assert(loaded.nodes.size() == 3);
    assert(loaded.nodeColors.size() == 3);
    assert(loaded.nodeColors[0].r == 255);
    assert(loaded.nodeColors[1].g == 255);
    assert(loaded.nodeColors[2].b == 255);
    
    assert(loaded.faceColors.size() == 1);
    assert(loaded.faceColors[0].r == 255);

    std::remove(filename.c_str());
    std::cout << "test_mesh_colors passed!" << std::endl;
}

void test_mesh_normals() {
    Mesh mesh;
    mesh.nodes = {{0,0,0}, {1,0,0}, {0,1,0}};
    mesh.triangles = {{0,1,2}};
    MeshGeometry geom(mesh);
    mesh.nodeNormals = geom.computeNodeNormals();
    assert(mesh.nodeNormals.size() == 3);
    assert(std::abs(mesh.nodeNormals[0].z - 1.0f) < 1e-6);

    const std::string offFile = "test_normals.off";
    bool success = MeshIO::save(offFile, mesh);
    assert(success);
    
    std::ifstream in(offFile);
    std::string header;
    in >> header;
    assert(header == "NOFF");
    in.close();
    std::remove(offFile.c_str());

    const std::string plyFile = "test_normals.ply";
    success = MeshIO::save(plyFile, mesh);
    assert(success);
    std::remove(plyFile.c_str());

    std::cout << "test_mesh_normals passed!" << std::endl;
}

void test_noff_save() {
    Mesh mesh;
    mesh.nodes = {{0,0,0}, {1,0,0}, {0,1,0}};
    mesh.triangles = {{0,1,2}};
    mesh.nodeNormals = {{0,0,1}, {0,0,1}, {0,0,1}};

    const std::string filename = "test_noff.off";
    bool success = MeshIO::save(filename, mesh);
    assert(success);

    std::ifstream file(filename);
    std::string line;
    std::getline(file, line);
    assert(line == "NOFF");
    
    std::getline(file, line); // nVerts nFaces nEdges
    std::getline(file, line); // first node
    std::stringstream ss(line);
    float val;
    int count = 0;
    while (ss >> val) count++;
    assert(count == 6);

    file.close();
    std::remove(filename.c_str());
    std::cout << "test_noff_save passed!" << std::endl;
}

int main() {
    test_noff_save();
    test_mesh_normals();
    test_mesh_colors();
    test_readOFF();
    test_getJetColor();
    test_computeFaceNormal();
    test_computeFaceCenter();
    test_computeBoundingSphere();
    test_createUVSphere();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
