#include "ShapeDiameter.h"
#include <tbb/parallel_for.h>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cstring>

ShapeDiameter::ShapeDiameter(const Mesh& mesh) : mesh(mesh) {
    buildScene();
}

ShapeDiameter::~ShapeDiameter() {}

void ShapeDiameter::compute(int numTheta, int numPhi, float coneAngle) {
    int numTris = (int)mesh.triangles.size();
    shapeDiameters.assign(numTris, 1e20f);

    tbb::parallel_for(0, numTris, [&](int triIdx) {
        std::vector<RayHit> hits;
        computeForFace(triIdx, hits, numTheta, numPhi, coneAngle);
        if (!hits.empty()) {
            std::vector<float> distances;
            for (const auto& h : hits) distances.push_back(h.distance);
            std::sort(distances.begin(), distances.end());
            shapeDiameters[triIdx] = distances[distances.size() / 2];
        }
    });
}

void ShapeDiameter::computeForFace(int triIdx, std::vector<RayHit>& hits, int numTheta, int numPhi, float coneAngle) const {
    hits.clear();
    if (triIdx < 0 || triIdx >= (int)mesh.triangles.size()) return;

    const Triangle& tri = mesh.triangles[triIdx];
    Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
    Vec3 inwardNormal = {-normal.x, -normal.y, -normal.z};
    Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

    Vec3 up = (std::abs(inwardNormal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
    Vec3 tangent = {inwardNormal.y * up.z - inwardNormal.z * up.y,
                    inwardNormal.z * up.x - inwardNormal.x * up.z,
                    inwardNormal.x * up.y - inwardNormal.y * up.x};
    float tLen = tangent.length();
    if (tLen > 0) { tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen; }
    
    Vec3 bitangent = {inwardNormal.y * tangent.z - inwardNormal.z * tangent.y,
                      inwardNormal.z * tangent.x - inwardNormal.x * tangent.z,
                      inwardNormal.x * tangent.y - inwardNormal.y * tangent.x};

    for (int i = 0; i < numTheta; ++i) {
        for (int j = 0; j < numPhi; ++j) {
            float theta = coneAngle * (i + 0.5f) / numTheta; 
            float phi = 2.0f * M_PI * (j + 0.5f) / numPhi;
            float sinT = sin(theta); float cosT = cos(theta);
            float sinP = sin(phi);   float cosP = cos(phi);
            Vec3 rayDir = {
                (tangent.x * cosP + bitangent.x * sinP) * sinT + inwardNormal.x * cosT,
                (tangent.y * cosP + bitangent.y * sinP) * sinT + inwardNormal.y * cosT,
                (tangent.z * cosP + bitangent.z * sinP) * sinT + inwardNormal.z * cosT
            };
            Ray ray;
            float epsilon = 0.0001f;
            ray.org = {faceCenter.x + inwardNormal.x * epsilon, faceCenter.y + inwardNormal.y * epsilon, faceCenter.z + inwardNormal.z * epsilon};
            ray.dir = rayDir;
            ray.tnear = 0.0f;
            ray.tfar = 1e10f;
            
            Hit hit = RayTracer::intersect(scene, ray);
            if(hit.hit) hits.push_back({(int)hit.primID, rayDir, hit.t});
        }
    }
}

void ShapeDiameter::getStats(float& minD, float& maxD, float& avgD) const {
    maxD = 0; minD = 1e20f;
    float sumDist = 0; int count = 0;
    for (float d : shapeDiameters) {
        if (d < 1e19f) {
            if (d > maxD) maxD = d;
            if (d < minD) minD = d;
            sumDist += d; count++;
        }
    }
    avgD = count > 0 ? sumDist / count : 0;
}

void ShapeDiameter::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}
