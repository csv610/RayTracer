#ifndef MATERIAL_REMOVER_H
#define MATERIAL_REMOVER_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

/**
 * @class MaterialRemover
 * @brief Simulates subtractive manufacturing processes.
 * 
 * This class models material removal from a stock volume using specific 
 * tool geometries. It identifies machining errors and surface deviations 
 * by comparing the desired target mesh with the simulated toolpath volume.
 */
class MaterialRemover {
public:
    MaterialRemover(const Mesh& mesh);
    ~MaterialRemover() = default;

    /**
     * @brief Simulates a 3-axis CNC machining process.
     * @param resolution Sampling density for the simulation grid.
     * @param toolRadius Radius of the cylindrical/ball tool.
     * @return A mesh representing the surface after material removal, 
     *         colored by deviation from the target.
     */
    Mesh simulateCnc(int resolution, float toolRadius) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
};

#endif // MATERIAL_REMOVER_H
