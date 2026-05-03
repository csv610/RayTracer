#include "VisibilityAnalyzer.h"
#include <cmath>
#include <algorithm>

VisibilityAnalyzer::VisibilityAnalyzer(const Mesh& mesh) : mesh(mesh) {
    buildScene();
    Vertex center;
    computeBoundingSphere(mesh, center, sphereRadius);
}

VisibilityAnalyzer::~VisibilityAnalyzer() {}

void VisibilityAnalyzer::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}

VisibilityAnalyzer::Result VisibilityAnalyzer::computeVisibility(int numTheta, int numPhi) const {
    Result res;
    res.colors.assign(mesh.triangles.size(), {255, 0, 0, 255});
    res.visibleCount = 0;

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
        for (int i = 0; i < numTheta; ++i) {
            for (int j = 0; j < numPhi; ++j) {
                float theta = M_PI * 0.5f * (i + 0.5f) / numTheta;
                float phi = 2.0f * M_PI * (j + 0.5f) / numPhi;
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
            res.visibleCount++;
            res.colors[triIdx] = {0, 255, 0, 255};
        }
    }
    return res;
}

Mesh VisibilityAnalyzer::Result::getColoredMesh(const Mesh& original) const {
    Mesh m = original;
    m.faceColors = colors;
    return m;
}
