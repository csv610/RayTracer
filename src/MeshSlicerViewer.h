#ifndef MESH_SLICER_VIEWER_H
#define MESH_SLICER_VIEWER_H

#include "Mesh.h"
#include "RayTracedSlicer.h"
#include <string>
#include <memory>

/**
 * @class MeshSlicerViewer
 * @brief Interactive visualization application for mesh slicing.
 * 
 * Built on SDL2, this class provides a graphical user interface to 
 * inspect a 3D mesh layer-by-layer using an Embree-backed ray tracer. 
 * Users can interactively scroll through slices along the X, Y, or Z axes, 
 * adjust resolution, and export specific cross-sections as PNG images.
 */
class MeshSlicerViewer {
public:
    MeshSlicerViewer();
    bool loadMesh(const char* filename);
    void setResolution(int res);
    void setAxis(SliceAxis axis);
    void run();

private:
    Mesh m_mesh;
    std::unique_ptr<RayTracedSlicer> m_slicer;
    int m_resolution;
    SliceAxis m_axis;
    int m_currentSlice;
    int m_totalSlices;
    bool m_dirty;

    void renderCurrentSlice();
    void saveCurrentSlice();
    std::string getOutputFilename() const;
};

#endif
