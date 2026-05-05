#include "MeshTopology.h"
#include <set>

MeshTopology::MeshTopology(const Mesh& mesh) : m_mesh(mesh) {
    buildConnectivity();
}

void MeshTopology::buildConnectivity() {
    m_nodeTriangles.assign(m_mesh.nodes.size(), {});
    for (size_t t = 0; t < m_mesh.triangles.size(); ++t) {
        const auto& tri = m_mesh.triangles[t];
        m_nodeTriangles[tri.v0].push_back((unsigned int)t);
        m_nodeTriangles[tri.v1].push_back((unsigned int)t);
        m_nodeTriangles[tri.v2].push_back((unsigned int)t);
    }

    m_nodeNeighbors.assign(m_mesh.nodes.size(), {});
    for (size_t v = 0; v < m_mesh.nodes.size(); ++v) {
        std::set<unsigned int> neighborSet;
        for (unsigned int t : m_nodeTriangles[v]) {
            const auto& tri = m_mesh.triangles[t];
            neighborSet.insert(tri.v0);
            neighborSet.insert(tri.v1);
            neighborSet.insert(tri.v2);
        }
        neighborSet.erase((unsigned int)v);
        m_nodeNeighbors[v].assign(neighborSet.begin(), neighborSet.end());
    }
}
