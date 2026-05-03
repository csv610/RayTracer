#include "AccessibilityAnalysis.h"
#include <tbb/parallel_for.h>
#include <cstring>
#include <cmath>

AccessibilityAnalysis::AccessibilityAnalysis(const Mesh& mesh) : mesh_(mesh) {
    scene_.addSharedMesh(mesh_);
    scene_.commit();
}

AccessibilityAnalysis::~AccessibilityAnalysis() {}

void AccessibilityAnalysis::analyze(float toolRadius) {
    Vertex center;
    float meshRadius;
    computeBoundingSphere(mesh_, center, meshRadius);
    float rayLength = meshRadius * 4.0f;
    float epsilon = meshRadius * 1e-4f;

    triColors_.resize(mesh_.triangles.size());
    inaccessibleCount_ = 0;

    tbb::parallel_for(size_t(0), mesh_.triangles.size(), [&](size_t i) {
        const Triangle& tri = mesh_.triangles[i];
        Vec3 normal = computeFaceNormal(mesh_.vertices[tri.v0], mesh_.vertices[tri.v1], mesh_.vertices[tri.v2]);
        Vec3 faceCenter = computeFaceCenter(mesh_.vertices[tri.v0], mesh_.vertices[tri.v1], mesh_.vertices[tri.v2]);

        if (normal.z < 0.05f) {
            triColors_[i] = {255, 0, 0, 255}; // Red
            return;
        }

        const int numPerimeterRays = 8;
        bool accessible = true;

        for (int j = -1; j < numPerimeterRays; ++j) {
            float dx = 0, dy = 0;
            if (j >= 0) {
                float angle = (2.0f * M_PI * j) / numPerimeterRays;
                dx = cos(angle) * toolRadius;
                dy = sin(angle) * toolRadius;
            }

            Ray ray;
            ray.org = {faceCenter.x + dx, faceCenter.y + dy, faceCenter.z + epsilon};
            ray.dir = {0, 0, 1.0f}; 
            ray.tnear = 0.0f;
            ray.tfar = rayLength;

            if (RayTracer::occluded(scene_, ray)) {
                accessible = false;
                break;
            }
        }

        if (accessible) {
            triColors_[i] = {0, 255, 0, 255}; // Green
        } else {
            triColors_[i] = {255, 0, 0, 255}; // Red
        }
    });

    for (const auto& c : triColors_) {
        if (c.r > 128) inaccessibleCount_++;
    }
}
