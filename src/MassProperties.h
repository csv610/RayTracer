#ifndef MASS_PROPERTIES_H
#define MASS_PROPERTIES_H

#include "Mesh.h"
#include "RayTracer.h"

/**
 * @class MassProperties
 * @brief Computes physical and inertial properties of a 3D mesh.
 * 
 * This class calculates the volume, center of mass, and the full inertia tensor 
 * of a closed mesh. It uses a high-precision grid-based integration 
 * (voxelization) approach powered by the Embree ray-tracing engine to 
 * ensure accuracy for complex, non-convex geometries.
 * 
 * Results are returned as a Properties struct containing double-precision metrics.
 */
class MassProperties {
public:
    struct Properties {
        double volume;
        Vec3 centerOfMass;
        double inertiaTensor[3][3];
    };

    MassProperties(const Mesh& mesh);
    ~MassProperties() = default;

    Properties compute(int resolution = 128) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
};

#endif // MASS_PROPERTIES_H
