#ifndef SIMULATION_SUITE_H
#define SIMULATION_SUITE_H

#include "mesh_utils.h"
#include <embree4/rtcore.h>
#include <vector>
#include <string>

class SimulationSuite {
public:
    SimulationSuite(const Mesh& mesh);
    ~SimulationSuite();

    // Voxelizer
    Mesh voxelize(int resolution) const;

    // Slicer
    void slice(int numLayers, int resolution, const std::string& prefix) const;

    // CNC Simulator
    Mesh simulateCnc(int resolution, float toolRadius) const;

private:
    const Mesh& mesh;
    RTCDevice device;
    RTCScene scene;
    void buildScene();
};

#endif
