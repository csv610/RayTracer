#ifndef MESH_SLICER_VIEWER_H
#define MESH_SLICER_VIEWER_H

#include "MeshSlicer.h"
#include <string>

class MeshSlicerViewer {
public:
    MeshSlicerViewer();
    bool loadMesh(const char* filename);
    void setResolution(int res);
    void setDirection(MeshSlicer::SliceDirection dir);
    void run();

private:
    Mesh m_mesh;
    int m_resolution;
    MeshSlicer::SliceDirection m_direction;
    int m_currentSlice;
    int m_totalSlices;
    bool m_dirty;

    void renderCurrentSlice();
    void saveCurrentSlice();
    std::string getOutputFilename() const;
};

#endif