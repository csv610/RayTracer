#include "RayTracer.h"
#include "MeshIO.h"
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <mesh.off> [output.off]\n", argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "mesh_output.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    Vertex center;
    float sphereRadius;
    computeBoundingSphere(mesh, center, sphereRadius);

    Scene scene;
    scene.addMesh(mesh);
    scene.commit();

    std::vector<Vec3> triColors(mesh.triangles.size(), {1.0f, 0.0f, 0.0f});
    int visibleCount = 0;

    const int NUM_THETA = 8;
    const int NUM_PHI = 16;

    for (size_t triIdx = 0; triIdx < mesh.triangles.size(); ++triIdx) {
        const Triangle& tri = mesh.triangles[triIdx];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        Vec3 up = (std::abs(normal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {normal.y * up.z - normal.z * up.y, normal.z * up.x - normal.x * up.z, normal.x * up.y - normal.y * up.x};
        float tLen = tangent.length();
        if(tLen > 0) { tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen; }
        Vec3 bitangent = {normal.y * tangent.z - normal.z * tangent.y, normal.z * tangent.x - normal.x * tangent.z, normal.x * tangent.y - normal.y * tangent.x};

        bool isVisible = false;
        for (int i = 0; i < NUM_THETA; ++i) {
            for (int j = 0; j < NUM_PHI; ++j) {
                float theta = M_PI * 0.5f * (i + 0.5f) / NUM_THETA;
                float phi = 2.0f * M_PI * (j + 0.5f) / NUM_PHI;
                Vec3 rayDir = {
                    (tangent.x * cos(phi) + bitangent.x * sin(phi)) * sin(theta) + normal.x * cos(theta),
                    (tangent.y * cos(phi) + bitangent.y * sin(phi)) * sin(theta) + normal.y * cos(theta),
                    (tangent.z * cos(phi) + bitangent.z * sin(phi)) * sin(theta) + normal.z * cos(theta)
                };

                Ray ray;
                ray.org = {faceCenter.x + normal.x * 0.0001f, faceCenter.y + normal.y * 0.0001f, faceCenter.z + normal.z * 0.0001f};
                ray.dir = rayDir;
                ray.tnear = 0.0f;
                ray.tfar = sphereRadius * 10.0f;

                if (!RayTracer::occluded(scene, ray)) {
                    isVisible = true;
                    break;
                }
            }
            if (isVisible) break;
        }

        if (isVisible) {
            visibleCount++;
            triColors[triIdx] = {0.0f, 1.0f, 0.0f};
        }
    }

    mesh.faceColors = triColors;
    MeshIO::save(outputFile, mesh);

    printf("Triangle visibility: %d visible (green), %zu occluded (red)\n", visibleCount, mesh.triangles.size() - visibleCount);
    return 0;
}
