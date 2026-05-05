#ifndef AUTO_ORIENTATION_OPTIMIZER_H
#define AUTO_ORIENTATION_OPTIMIZER_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class AutoOrientationOptimizer
 * @brief Optimizes the orientation of a mesh for additive manufacturing.
 * 
 * This class identifies the optimal 'up' direction to minimize the volume
 * of support material required for 3D printing. It samples multiple
 * orientations on a sphere and calculates the projected support volume
 * for each using ray-tracing.
 * 
 * Results are returned as a ranked list of orientations and their
 * corresponding support volumes.
 */
class AutoOrientationOptimizer {
public:
    struct Result {
        Quaternion orientation;
        float supportVolume;
    };

    AutoOrientationOptimizer(const Mesh& mesh);
    ~AutoOrientationOptimizer();

    std::vector<Result> optimize(int numSamples = 32) const;

    /**
     * @brief Calculates the projected support volume for a given orientation.
     */
    float calculateSupportVolume(const Quaternion& q) const;

    /**
     * @brief Saves a rendered visualization of the mesh in the specified orientation.
     * @param res The optimization result containing the target 'up' vector.
     * @param filename Path to save the PNG image.
     */
    void saveOptimalViewPNG(const Result& res, const char* filename) const;

private:
    const Mesh& mesh;
    Scene scene;
    float meshDiag;

    void buildScene();
};

#endif
