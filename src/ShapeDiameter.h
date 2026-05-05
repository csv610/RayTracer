#ifndef SHAPE_DIAMETER_H
#define SHAPE_DIAMETER_H

#include <vector>
#include <string>
#include "Mesh.h"
#include "RayTracer.h"

/**
 * @class ShapeDiameter
 * @brief Implements the Shape Diameter Function (SDF) for local thickness analysis.
 * 
 * The SDF measures the local "diameter" of a 3D shape at any point on its 
 * surface. This class computes the SDF for each face by casting a cone 
 * of rays in the inward-pointing normal direction and calculating the 
 * median intersection distance. 
 * 
 * This metric is useful for shape segmentation, skeletonization, and 
 * identifying structural characteristics.
 */
class ShapeDiameter {
public:
    struct RayHit {
        int primID;
        Vec3 dir;
        float distance;
    };

    ShapeDiameter(const Mesh& mesh);
    ~ShapeDiameter();

    const std::vector<float>& getDiameters() const { return shapeDiameters; }
    void computeForFace(int triIdx, std::vector<RayHit>& hits, int numTheta = 4, int numPhi = 8, float coneAngle = 0.5f) const;

    void compute(int numTheta = 4, int numPhi = 8, float coneAngle = 0.5f);
    const Scene& getScene() const { return scene; }
    void getStats(float& minD, float& maxD, float& avgD) const;

private:
    const Mesh& mesh;
    std::vector<float> shapeDiameters;
    Scene scene;

    void buildScene();
};

#endif // SHAPE_DIAMETER_H
