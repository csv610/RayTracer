#ifndef CAD_FEATURE_DETECTOR_H
#define CAD_FEATURE_DETECTOR_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class CADFeatureDetector
 * @brief Identifies mechanical CAD features like Pockets, Slots, and Cavities.
 * 
 * This component uses hemispherical ray probing to analyze the surrounding
 * volume of each node. It classifies regions based on directional occlusion
 * density and anisotropy.
 */
class CADFeatureDetector {
public:
    enum class FeatureType {
        NONE,
        POCKET,
        SLOT,
        CAVITY,
        THROUGH_HOLE
    };

    struct Result {
        std::vector<FeatureType> nodeFeatures;
        
        /**
         * @brief Returns a mesh where nodes are colored by detected feature.
         *        Grey: None, Blue: Pocket, Yellow: Slot, Red: Cavity, Green: Through Hole.
         */
        Mesh getColoredMesh(const Mesh& original) const;
    };

    CADFeatureDetector(const Mesh& mesh);
    ~CADFeatureDetector();

    /**
     * @brief Performs the feature detection analysis.
     * @param numRays Number of rays per node for hemispherical probing (default 32).
     * @param searchScale Max distance to search for features (relative to mesh size).
     */
    Result detectFeatures(int numRays = 32, float searchScale = 0.1f) const;

    /**
     * @brief Computes exposure values specifically for detected pocket regions.
     * @return Vector of exposure values per node. Non-pocket nodes will have 1.0 (full exposure).
     */
    std::vector<float> computePocketExposure(int samples = 64) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    float m_meshDiag;
    std::vector<Vec3> m_nodeNormals;

    void buildScene();
    void computeNormals();
};

#endif // CAD_FEATURE_DETECTOR_H
