#include "MeshIO.h"
#include "InsideOutsideKernel.h"
#include <iostream>
#include <vector>

int main() {
    Mesh mesh;
    if (!MeshIO::load("/tmp/cube.off", mesh)) {
        std::cerr << "Failed to load /tmp/cube.off" << std::endl;
        return 1;
    }

    if (mesh.vertices.empty()) {
        std::cerr << "Mesh has no vertices." << std::endl;
        return 1;
    }

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 center = {(bbox.min.x + bbox.max.x)*0.5f, (bbox.min.y + bbox.max.y)*0.5f, (bbox.min.z + bbox.max.z)*0.5f};
    Vec3 outside = {bbox.max.x + 10.0f, bbox.max.y + 10.0f, bbox.max.z + 10.0f};
    
    const auto& v0 = mesh.vertices[0];
    Vec3 onSurface = {v0.x, v0.y, v0.z};

    InsideOutsideKernel kernel(mesh);

    std::cout << "Testing /tmp/cube.off" << std::endl;
    std::cout << "BBox: [" << bbox.min.x << "," << bbox.min.y << "," << bbox.min.z << "] to [" 
              << bbox.max.x << "," << bbox.max.y << "," << bbox.max.z << "]" << std::endl;

    int resCenter = kernel.classify(center);
    int resOutside = kernel.classify(outside);
    int resSurface = kernel.classify(onSurface);

    std::cout << "Point at Center (" << center.x << ", " << center.y << ", " << center.z << "): " << resCenter << " (Expected: -1)" << std::endl;
    std::cout << "Point Outside (" << outside.x << ", " << outside.y << ", " << outside.z << "): " << resOutside << " (Expected: 1)" << std::endl;
    std::cout << "Point on Vertex (" << onSurface.x << ", " << onSurface.y << ", " << onSurface.z << "): " << resSurface << " (Expected: 0)" << std::endl;

    if (resCenter == -1 && resOutside == 1 && resSurface == 0) {
        std::cout << "KERNEL TEST PASSED" << std::endl;
    } else {
        std::cout << "KERNEL TEST FAILED" << std::endl;
    }

    return 0;
}
