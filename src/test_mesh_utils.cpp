#include "mesh_utils.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>

void test_computeFaceNormal() {
    Vertex v0 = {0, 0, 0};
    Vertex v1 = {1, 0, 0};
    Vertex v2 = {0, 1, 0};
    Vec3 n = computeFaceNormal(v0, v1, v2);
    assert(std::abs(n.x) < 1e-6);
    assert(std::abs(n.y) < 1e-6);
    assert(std::abs(n.z - 1.0f) < 1e-6);
    std::cout << "test_computeFaceNormal passed!" << std::endl;
}

void test_computeFaceCenter() {
    Vertex v0 = {0, 0, 0};
    Vertex v1 = {3, 0, 0};
    Vertex v2 = {0, 3, 0};
    Vec3 c = computeFaceCenter(v0, v1, v2);
    assert(std::abs(c.x - 1.0f) < 1e-6);
    assert(std::abs(c.y - 1.0f) < 1e-6);
    assert(std::abs(c.z) < 1e-6);
    std::cout << "test_computeFaceCenter passed!" << std::endl;
}

void test_computeBoundingSphere() {
    std::vector<Vertex> vertices = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}
    };
    Vertex center;
    float radius;
    computeBoundingSphere(vertices, center, radius);
    assert(std::abs(center.x) < 1e-6);
    assert(std::abs(center.y) < 1e-6);
    assert(std::abs(center.z) < 1e-6);
    assert(std::abs(radius - 1.0f) < 1e-6);
    std::cout << "test_computeBoundingSphere passed!" << std::endl;
}

void test_createUVSphere() {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    createUVSphere(vertices, triangles, 10, 10, 1.0f);
    assert(!vertices.empty());
    assert(!triangles.empty());
    std::cout << "test_createUVSphere passed!" << std::endl;
}

void test_readOFF() {
    const char* filename = "test_temp.off";
    std::ofstream file(filename);
    file << "OFF\n3 1 0\n0 0 0\n1 0 0\n0 1 0\n3 0 1 2\n";
    file.close();

    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    bool success = readOFF(filename, vertices, triangles);
    assert(success);
    assert(vertices.size() == 3);
    assert(triangles.size() == 1);
    assert(triangles[0].v0 == 0);
    assert(triangles[0].v1 == 1);
    assert(triangles[0].v2 == 2);
    
    std::remove(filename);
    std::cout << "test_readOFF passed!" << std::endl;
}

void test_getJetColor() {
    Vec3 c0 = getJetColor(0.0f);
    assert(std::abs(c0.x) < 1e-6);
    assert(std::abs(c0.y) < 1e-6);
    assert(std::abs(c0.z - 0.5f) < 1e-6); // Jet at 0 is blue-ish

    Vec3 c1 = getJetColor(1.0f);
    assert(std::abs(c1.x - 0.5f) < 1e-6); // Jet at 1 is red-ish
    assert(std::abs(c1.y) < 1e-6);
    assert(std::abs(c1.z) < 1e-6);
    
    std::cout << "test_getJetColor passed!" << std::endl;
}

int main() {
    test_readOFF();
    test_getJetColor();
    test_computeFaceNormal();
    test_computeFaceCenter();
    test_computeBoundingSphere();
    test_createUVSphere();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
