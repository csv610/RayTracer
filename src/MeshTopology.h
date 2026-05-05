#ifndef MESH_TOPOLOGY_H
#define MESH_TOPOLOGY_H

#include "Mesh.h"
#include <vector>

/**
 * @class MeshTopology
 * @brief Manages the connectivity and adjacency relationships of a mesh.
 * 
 * This class builds and maintains the "skeleton" of the mesh:
 * - Node-to-Triangle maps (which triangles use this node?)
 * - Node-to-Node adjacency (neighboring nodes)
 */
class MeshTopology {
public:
    MeshTopology(const Mesh& mesh);

    const std::vector<std::vector<unsigned int>>& getNodeTriangles() const { return m_nodeTriangles; }
    const std::vector<std::vector<unsigned int>>& getNodeNeighbors() const { return m_nodeNeighbors; }

private:
    const Mesh& m_mesh;
    std::vector<std::vector<unsigned int>> m_nodeTriangles;
    std::vector<std::vector<unsigned int>> m_nodeNeighbors;

    void buildConnectivity();
};

#endif // MESH_TOPOLOGY_H
