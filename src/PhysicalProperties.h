#ifndef PHYSICAL_PROPERTIES_H
#define PHYSICAL_PROPERTIES_H

#include "mesh_utils.h"
#include <embree4/rtcore.h>

class PhysicalProperties {
public:
    struct Properties {
        double volume;
        Vec3 centerOfMass;
        double inertiaTensor[3][3];
    };

    PhysicalProperties(const Mesh& mesh);
    ~PhysicalProperties();

    Properties compute(int resolution = 128) const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    void buildScene();
};

#endif
