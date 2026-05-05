#include "CADFeatureDetector.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <cmath>
#include <algorithm>
#include <numeric>

CADFeatureDetector::CADFeatureDetector(const Mesh& mesh) : m_mesh(mesh) {
    buildScene();
    computeNormals();
    MeshGeometry geom(m_mesh);
    m_meshDiag = geom.computeAABB().size().length();
}

CADFeatureDetector::~CADFeatureDetector() {}

void CADFeatureDetector::buildScene() {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
}

void CADFeatureDetector::computeNormals() {
    m_nodeNormals.assign(m_mesh.nodes.size(), {0, 0, 0});
    for (const auto& tri : m_mesh.triangles) {
        Vec3 n = MeshGeometry::computeFaceNormal(m_mesh.nodes[tri.v0], m_mesh.nodes[tri.v1], m_mesh.nodes[tri.v2]);
        m_nodeNormals[tri.v0].x += n.x; m_nodeNormals[tri.v0].y += n.y; m_nodeNormals[tri.v0].z += n.z;
        m_nodeNormals[tri.v1].x += n.x; m_nodeNormals[tri.v1].y += n.y; m_nodeNormals[tri.v1].z += n.z;
        m_nodeNormals[tri.v2].x += n.x; m_nodeNormals[tri.v2].y += n.y; m_nodeNormals[tri.v2].z += n.z;
    }
    for (auto& n : m_nodeNormals) {
        float l = n.length();
        if (l > 0) { n.x /= l; n.y /= l; n.z /= l; }
    }
}

CADFeatureDetector::Result CADFeatureDetector::detectFeatures(int numRays, float searchScale) const {
    Result res;
    res.nodeFeatures.assign(m_mesh.nodes.size(), FeatureType::NONE);
    
    float searchRadius = m_meshDiag * searchScale;
    float epsilon = m_meshDiag * 1e-4f;

    tbb::parallel_for(size_t(0), m_mesh.nodes.size(), [&](size_t vidx) {
        const auto& p = m_mesh.nodes[vidx];
        Vec3 n = m_nodeNormals[vidx];

        // Setup local coordinate system
        Vec3 up = (std::abs(n.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {n.y * up.z - n.z * up.y, n.z * up.x - n.x * up.z, n.x * up.y - n.y * up.x};
        float tLen = tangent.length();
        if (tLen > 0) { tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen; }
        Vec3 bitangent = {n.y * tangent.z - n.z * tangent.y, n.z * tangent.x - n.x * tangent.z, n.x * tangent.y - n.y * tangent.x};

        std::vector<float> distances;
        distances.reserve(numRays);
        int hits = 0;

        for (int i = 0; i < numRays; ++i) {
            // Fibonacci hemisphere sampling
            float phi = acos(1.0f - (float)i / numRays);
            float theta = 2.0f * M_PI * 1.618033f * i;

            Vec3 rayDir = {
                (tangent.x * cos(theta) + bitangent.x * sin(theta)) * sin(phi) + n.x * cos(phi),
                (tangent.y * cos(theta) + bitangent.y * sin(theta)) * sin(phi) + n.y * cos(phi),
                (tangent.z * cos(theta) + bitangent.z * sin(theta)) * sin(phi) + n.z * cos(phi)
            };

            Ray ray;
            ray.org = {p.x + n.x * epsilon, p.y + n.y * epsilon, p.z + n.z * epsilon};
            ray.dir = rayDir;
            ray.tnear = 0.0f;
            ray.tfar = searchRadius;

            Hit hit = RayTracer::intersect(m_scene, ray);
            if (hit.hit) {
                distances.push_back(hit.t);
                hits++;
            } else {
                distances.push_back(searchRadius);
            }
        }

        float occlusionRatio = (float)hits / numRays;
        
        // Calculate Anisotropy (variance in hit patterns)
        float minDist = *std::min_element(distances.begin(), distances.end());
        float maxDist = *std::max_element(distances.begin(), distances.end());
        float range = maxDist - minDist;

        if (occlusionRatio > 0.9f) {
            res.nodeFeatures[vidx] = FeatureType::CAVITY;
        } else if (occlusionRatio > 0.4f) {
            // Check for THROUGH HOLE: High occlusion radially, but escapes at poles
            // A through hole node typically sees "infinity" (searchRadius) 
            // in two opposite directions.
            int escapePaths = 0;
            for (int i = 0; i < numRays; ++i) {
                if (distances[i] >= searchRadius * 0.99f) escapePaths++;
            }

            if (escapePaths >= 2 && range > searchRadius * 0.8f) {
                res.nodeFeatures[vidx] = FeatureType::THROUGH_HOLE;
            } else if (range > searchRadius * 0.6f) {
                res.nodeFeatures[vidx] = FeatureType::SLOT;
            } else {
                res.nodeFeatures[vidx] = FeatureType::POCKET;
            }
        }
    });

    return res;
}

std::vector<float> CADFeatureDetector::computePocketExposure(int samples) const {
    Result features = detectFeatures();
    std::vector<float> exposures(m_mesh.nodes.size(), 1.0f);
    float epsilon = m_meshDiag * 1e-4f;
    float rayLength = m_meshDiag * 2.0f;

    tbb::parallel_for(size_t(0), m_mesh.nodes.size(), [&](size_t vidx) {
        if (features.nodeFeatures[vidx] != FeatureType::POCKET && 
            features.nodeFeatures[vidx] != FeatureType::SLOT) {
            return;
        }

        Vec3 n = m_nodeNormals[vidx];
        const auto& p = m_mesh.nodes[vidx];

        Vec3 up = (std::abs(n.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {n.y * up.z - n.z * up.y, n.z * up.x - n.x * up.z, n.x * up.y - n.y * up.x};
        float tLen = tangent.length();
        if (tLen > 0) { tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen; }
        Vec3 bitangent = {n.y * tangent.z - n.z * tangent.y, n.z * tangent.x - n.x * tangent.z, n.x * tangent.y - n.y * tangent.x};

        int hits = 0;
        for (int s = 0; s < samples; ++s) {
            float u = (float)s / samples;
            unsigned int bits = (s << 16) | (s >> 16);
            bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
            bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
            bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
            bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
            float v = (float)bits * 2.3283064365386963e-10;

            float phi = 2.0f * (float)M_PI * u;
            float cosTheta = v;
            float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
            
            Vec3 localDir = {std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta};
            Vec3 rayDir = {
                tangent.x * localDir.x + bitangent.x * localDir.y + n.x * localDir.z,
                tangent.y * localDir.x + bitangent.y * localDir.y + n.y * localDir.z,
                tangent.z * localDir.x + bitangent.z * localDir.y + n.z * localDir.z
            };

            Ray ray;
            ray.org = {p.x + n.x * epsilon, p.y + n.y * epsilon, p.z + n.z * epsilon};
            ray.dir = rayDir;
            ray.tnear = 0.0f;
            ray.tfar = rayLength;

            if (RayTracer::occluded(m_scene, ray)) hits++;
        }
        exposures[vidx] = 1.0f - (float)hits / samples;
    });

    return exposures;
}

Mesh CADFeatureDetector::Result::getColoredMesh(const Mesh& original) const {
    Mesh m = original;
    m.nodeColors.assign(m.nodes.size(), {180, 180, 180, 255}); // Base grey

    for (size_t i = 0; i < nodeFeatures.size(); ++i) {
        switch (nodeFeatures[i]) {
            case FeatureType::POCKET:
                m.nodeColors[i] = {0, 0, 255, 255}; // Blue
                break;
            case FeatureType::SLOT:
                m.nodeColors[i] = {255, 255, 0, 255}; // Yellow
                break;
            case FeatureType::CAVITY:
                m.nodeColors[i] = {255, 0, 0, 255}; // Red
                break;
            case FeatureType::THROUGH_HOLE:
                m.nodeColors[i] = {0, 255, 0, 255}; // Green
                break;
            case FeatureType::NONE:
            default:
                break;
        }
    }
    return m;
}
