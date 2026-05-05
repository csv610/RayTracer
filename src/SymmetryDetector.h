#ifndef SYMMETRY_DETECTOR_H
#define SYMMETRY_DETECTOR_H

#include "Mesh.h"
#include "RayTracer.h"
#include "SampleSurfacePoints.h"
#include <vector>

struct Plane {
    Vec3 normal;
    float distance; // n.p = d

    Vec3 reflectPoint(const Vec3& p) const {
        float d = (normal.x * p.x + normal.y * p.y + normal.z * p.z) - distance;
        return {p.x - 2 * d * normal.x, p.y - 2 * d * normal.y, p.z - 2 * d * normal.z};
    }

    Vec3 reflectVector(const Vec3& v) const {
        float d = (normal.x * v.x + normal.y * v.y + normal.z * v.z);
        return {v.x - 2 * d * normal.x, v.y - 2 * d * normal.y, v.z - 2 * d * normal.z};
    }
};

/**
 * @class SymmetryDetector
 * @brief Detects reflective and planar symmetries in 3D models.
 * 
 * This component identifies the primary planes of symmetry for a given mesh. 
 * It uses Principal Component Analysis (PCA) to find initial candidates 
 * and then performs an exhaustive geometric validation by measuring 
 * the alignment of reflected surface samples.
 * 
 * It can classify symmetry quality as STRONG, MODERATE, or NONE.
 */
class SymmetryDetector {
public:
    enum class Quality { NONE, MODERATE, STRONG };

    struct Result {
        Plane bestPlane;
        float bestScore;
        Quality quality;
        std::vector<std::pair<Plane, float>> candidates;
    };

    SymmetryDetector(const Mesh& mesh);
    ~SymmetryDetector() = default;

    /**
     * @brief Returns a symmetry score (lower is better, 0 is perfect symmetry).
     */
    float checkSymmetry(const Plane& plane) const;

    /**
     * @brief Finds candidate planes based on principal axes.
     */
    std::vector<Plane> findCandidatePlanes() const;

    /**
     * @brief High-level symmetry detection.
     */
    Result detectSymmetry() const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    float m_meshDiagonal;
    std::vector<SampledPoint> m_preSampledPoints;

    void preSample(int numSamples);
};

#endif // SYMMETRY_DETECTOR_H
