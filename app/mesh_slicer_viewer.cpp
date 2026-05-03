#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <cmath>
#include "mesh_utils.h"
#include "MeshIO.h"
#include "RayTracer.h"
#include "RayTracedSlicer.h"

#include <QApplication>
#include <QGLViewer/qglviewer.h>
#include <QKeyEvent>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QImage>
#include <QColor>

class MeshSlicerVis : public QGLViewer {
public:
    MeshSlicerVis(const std::string& filename, int numLayers, int res) 
        : inputFile(filename), numLayers(numLayers), resolution(res) {}
    ~MeshSlicerVis() {
        if (vbo_vertices) glDeleteBuffers(1, &vbo_vertices);
        if (vbo_indices) glDeleteBuffers(1, &vbo_indices);
        if (vbo_colors) glDeleteBuffers(1, &vbo_colors);
    }

protected:
    virtual void draw() override;
    virtual void init() override;
    virtual void keyPressEvent(QKeyEvent* e) override;

private:
    std::string inputFile;
    Mesh mesh;
    std::unique_ptr<RayTracedSlicer> slicer;
    SlicerLayer currentLayerData;
    
    int currentLayerIdx = 0;
    int numLayers = 50;
    int resolution = 128;
    SliceAxis currentAxis = SliceAxis::Z;

    bool showWireframe = false;
    bool showFilled = true;
    bool showSliceGrid = false;
    bool meshVisible = true;
    bool showBBox = true;
    bool showRays = false;
    bool showRaySources = true;

    AABB bbox;

    GLuint vbo_vertices = 0;
    GLuint vbo_indices = 0;
    GLuint vbo_colors = 0;
    GLuint vbo_mesh_normals = 0;
    GLuint vbo_ray_sources = 0;
    GLuint sliceTexture = 0;

    void loadMesh();
    void updateCurrentLayer();
    void drawMesh();
    void drawSlice();
    void drawBoundingBox();
    void drawRaySources();
    void drawSourcePlane();
    void drawRays();
    void saveSlice();
    void setupVBOs();
    void computeBoundingBox();
};

void MeshSlicerVis::loadMesh() {
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        exit(1);
    }
    computeVertexNormals(mesh);
    computeBoundingBox();
    slicer = std::make_unique<RayTracedSlicer>(mesh);
}

void MeshSlicerVis::computeBoundingBox() {
    bbox = AABB();
    for (const auto& v : mesh.vertices) {
        bbox.expand(v);
    }
    bbox.pad(0.005f); // 0.5% on each side = 1% total expansion
}

void MeshSlicerVis::updateCurrentLayer() {
    if (!slicer) return;
    currentLayerData = slicer->computeLayer(currentLayerIdx, numLayers, resolution, currentAxis);
    std::cout << "Layer " << (currentLayerIdx+1) << "/" << numLayers 
             << " @ z=" << currentLayerData.z 
             << " (" << (currentAxis == SliceAxis::X ? 'X' : currentAxis == SliceAxis::Y ? 'Y' : 'Z') << ")"
             << " res=" << currentLayerData.texWidth << "x" << currentLayerData.texHeight << std::endl;
    setupVBOs();
    update();
}

void MeshSlicerVis::init() {
    loadMesh();
    updateCurrentLayer();

    camera()->setSceneCenter(qglviewer::Vec(bbox.min.x + (bbox.max.x - bbox.min.x) * 0.5f,
                                       bbox.min.y + (bbox.max.y - bbox.min.y) * 0.5f,
                                       bbox.min.z + (bbox.max.z - bbox.min.z) * 0.5f));
    camera()->setSceneRadius((bbox.max.x - bbox.min.x) * 2.0f);
    camera()->showEntireScene();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat lightPos[] = {1.0f, 1.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    
    std::cout << "Mesh Slicer loaded. Press SPACE/Up/Down/Left/Right to cycle layer." << std::endl;
    std::cout << "Navigation: PgUp/PgDn = 10% jump, Home/End = First/Last layer" << std::endl;
    std::cout << "Resolution: +/- = adjust, 1-9 = presets (64 to 2048)" << std::endl;
    std::cout << "Toggles:    F=slice fill, W=wireframe, G=grid, T=mesh, B=bbox, E=sources, D=rays" << std::endl;
    std::cout << "Camera:     R=reset view, O=perspective/ortho toggle" << std::endl;
    std::cout << "Slicing:    X/Y/Z = change axis, S = save current slice" << std::endl;
    std::cout << "Layers: " << numLayers << ", Resolution: " << resolution << std::endl;
}

void MeshSlicerVis::setupVBOs() {
    if (currentLayerData.vertices.empty()) {
        return;
    }
    
    if (!vbo_vertices) glGenBuffers(1, &vbo_vertices);
    if (!vbo_indices) glGenBuffers(1, &vbo_indices);
    if (!vbo_colors) glGenBuffers(1, &vbo_colors);
    if (!vbo_ray_sources) glGenBuffers(1, &vbo_ray_sources);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, currentLayerData.vertices.size() * sizeof(Vertex),
                currentLayerData.vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    if (!currentLayerData.triangles.empty()) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, currentLayerData.triangles.size() * sizeof(Triangle),
                    currentLayerData.triangles.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, currentLayerData.vertexColors.size() * sizeof(Color4b),
                currentLayerData.vertexColors.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ray_sources);
    glBufferData(GL_ARRAY_BUFFER, currentLayerData.raySources.size() * sizeof(Vertex),
                currentLayerData.raySources.data(), GL_DYNAMIC_DRAW);
    }
void MeshSlicerVis::draw() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.3f, 0.3f);
    glLineWidth(3.0f);
    float sliceZ = currentLayerData.z;
    if (currentAxis == SliceAxis::X) {
        glBegin(GL_LINE_LOOP);
        glVertex3f(sliceZ, bbox.min.y, bbox.min.z);
        glVertex3f(sliceZ, bbox.max.y, bbox.min.z);
        glVertex3f(sliceZ, bbox.max.y, bbox.max.z);
        glVertex3f(sliceZ, bbox.min.y, bbox.max.z);
        glEnd();
    } else if (currentAxis == SliceAxis::Y) {
        glBegin(GL_LINE_LOOP);
        glVertex3f(bbox.min.x, sliceZ, bbox.min.z);
        glVertex3f(bbox.max.x, sliceZ, bbox.min.z);
        glVertex3f(bbox.max.x, sliceZ, bbox.max.z);
        glVertex3f(bbox.min.x, sliceZ, bbox.max.z);
        glEnd();
    } else {
        glBegin(GL_LINE_LOOP);
        glVertex3f(bbox.min.x, bbox.min.y, sliceZ);
        glVertex3f(bbox.max.x, bbox.min.y, sliceZ);
        glVertex3f(bbox.max.x, bbox.max.y, sliceZ);
        glVertex3f(bbox.min.x, bbox.max.y, sliceZ);
        glEnd();
    }
    glEnable(GL_LIGHTING);

    if (meshVisible) {
        drawMesh();
    }

    if (showBBox) {
        drawBoundingBox();
    }

    if (showRaySources) {
        drawSourcePlane();
        drawRaySources();
    }

    if (showRays) {
        drawRays();
    }

    if (showFilled || showWireframe || showSliceGrid) {
        drawSlice();
    }
}

void MeshSlicerVis::drawRays() {
    if (currentLayerData.raySources.empty() || currentLayerData.vertices.empty()) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 0.0f, 0.4f); // Yellow with 40% alpha

    glBegin(GL_LINES);
    int step = std::max(1, (int)(currentLayerData.raySources.size() / 1000));
    for (size_t i = 0; i < currentLayerData.raySources.size(); i += step) {
        const auto& s = currentLayerData.raySources[i];
        const auto& v = currentLayerData.vertices[i];
        glVertex3f(s.x, s.y, s.z);
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}
void MeshSlicerVis::drawRaySources() {
    if (currentLayerData.raySources.empty() || !vbo_ray_sources) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    glPointSize(4.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ray_sources);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glDrawArrays(GL_POINTS, 0, currentLayerData.raySources.size());
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void MeshSlicerVis::drawSourcePlane() {
    float minX = bbox.min.x; float maxX = bbox.max.x;
    float minY = bbox.min.y; float maxY = bbox.max.y;
    float minZ = bbox.min.z; float maxZ = bbox.max.z;
    Vec3 size = bbox.size();

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 0.0f, 0.2f); // Transparent yellow

    glBegin(GL_QUADS);
    if (currentAxis == SliceAxis::X) {
        float sliceDist = size.x / (numLayers + 1);
        float ex = minX - 2.0f * sliceDist;
        glVertex3f(ex, minY, minZ); glVertex3f(ex, maxY, minZ);
        glVertex3f(ex, maxY, maxZ); glVertex3f(ex, minY, maxZ);
    } else if (currentAxis == SliceAxis::Y) {
        float sliceDist = size.y / (numLayers + 1);
        float ey = minY - 2.0f * sliceDist;
        glVertex3f(minX, ey, minZ); glVertex3f(maxX, ey, minZ);
        glVertex3f(maxX, ey, maxZ); glVertex3f(minX, ey, maxZ);
    } else {
        float sliceDist = size.z / (numLayers + 1);
        float ez = minZ - 2.0f * sliceDist;
        glVertex3f(minX, minY, ez); glVertex3f(maxX, minY, ez);
        glVertex3f(maxX, maxY, ez); glVertex3f(minX, maxY, ez);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void MeshSlicerVis::drawBoundingBox() {
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);

    glBegin(GL_LINES);
    // X-axis lines (Red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(bbox.min.x, bbox.min.y, bbox.min.z); glVertex3f(bbox.max.x, bbox.min.y, bbox.min.z);
    glVertex3f(bbox.min.x, bbox.max.y, bbox.min.z); glVertex3f(bbox.max.x, bbox.max.y, bbox.min.z);
    glVertex3f(bbox.min.x, bbox.min.y, bbox.max.z); glVertex3f(bbox.max.x, bbox.min.y, bbox.max.z);
    glVertex3f(bbox.min.x, bbox.max.y, bbox.max.z); glVertex3f(bbox.max.x, bbox.max.y, bbox.max.z);

    // Y-axis lines (Green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(bbox.min.x, bbox.min.y, bbox.min.z); glVertex3f(bbox.min.x, bbox.max.y, bbox.min.z);
    glVertex3f(bbox.max.x, bbox.min.y, bbox.min.z); glVertex3f(bbox.max.x, bbox.max.y, bbox.min.z);
    glVertex3f(bbox.min.x, bbox.min.y, bbox.max.z); glVertex3f(bbox.min.x, bbox.max.y, bbox.max.z);
    glVertex3f(bbox.max.x, bbox.min.y, bbox.max.z); glVertex3f(bbox.max.x, bbox.max.y, bbox.max.z);

    // Z-axis lines (Blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(bbox.min.x, bbox.min.y, bbox.min.z); glVertex3f(bbox.min.x, bbox.min.y, bbox.max.z);
    glVertex3f(bbox.max.x, bbox.min.y, bbox.min.z); glVertex3f(bbox.max.x, bbox.min.y, bbox.max.z);
    glVertex3f(bbox.max.x, bbox.max.y, bbox.min.z); glVertex3f(bbox.max.x, bbox.max.y, bbox.max.z);
    glVertex3f(bbox.min.x, bbox.max.y, bbox.min.z); glVertex3f(bbox.min.x, bbox.max.y, bbox.max.z);
    glEnd();
    glEnable(GL_LIGHTING);
}

void MeshSlicerVis::drawMesh() {
    if (mesh.vertices.empty() || mesh.triangles.empty()) return;

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    
    GLfloat ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat specular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    static GLuint meshVBO = 0;
    if (!meshVBO) {
        glGenBuffers(1, &meshVBO);
        glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);
    }
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    if (!vbo_mesh_normals) {
        glGenBuffers(1, &vbo_mesh_normals);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_mesh_normals);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertexNormals.size() * sizeof(Vec3), mesh.vertexNormals.data(), GL_STATIC_DRAW);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_mesh_normals);
    glNormalPointer(GL_FLOAT, 0, 0);
    
    static GLuint meshIBO = 0;
    if (!meshIBO) {
        glGenBuffers(1, &meshIBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.triangles.size() * sizeof(Triangle), mesh.triangles.data(), GL_STATIC_DRAW);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIBO);
    
    glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
    glDrawElements(GL_TRIANGLES, mesh.triangles.size() * 3, GL_UNSIGNED_INT, 0);
    
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_COLOR_MATERIAL);
}

void MeshSlicerVis::drawSlice() {
    if (currentLayerData.vertices.empty()) return;
    if (!showFilled) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    
    float sliceZ = currentLayerData.z;
    float offset = bbox.size().length() * 1e-4f;
    int w = currentLayerData.texWidth;
    int h = currentLayerData.texHeight;
    
    float minU, maxU, minV, maxV;
    switch (currentAxis) {
        case SliceAxis::X:
            minU = bbox.min.y; maxU = bbox.max.y;
            minV = bbox.min.z; maxV = bbox.max.z;
            break;
        case SliceAxis::Y:
            minU = bbox.min.x; maxU = bbox.max.x;
            minV = bbox.min.z; maxV = bbox.max.z;
            break;
        case SliceAxis::Z:
            minU = bbox.min.x; maxU = bbox.max.x;
            minV = bbox.min.y; maxV = bbox.max.y;
            break;
    }
    
    float du = (maxU - minU) / w;
    float dv = (maxV - minV) / h;
    
    for (int vy = 0; vy < h; ++vy) {
        for (int ux = 0; ux < w; ++ux) {
            int idx = (vy * w + ux) * 3;
            unsigned char c = currentLayerData.textureData[idx];
            
            if (c >= 128) glColor3f(1.0f, 1.0f, 1.0f);
            else glColor3f(0.15f, 0.15f, 0.15f);
            
            float x0 = minU + ux * du;
            float y0 = minV + vy * dv;
            float x1 = x0 + du;
            float y1 = y0 + dv;
            
            glBegin(GL_QUADS);
            if (currentAxis == SliceAxis::X) {
                glVertex3f(sliceZ + offset, x0, y0);
                glVertex3f(sliceZ + offset, x1, y0);
                glVertex3f(sliceZ + offset, x1, y1);
                glVertex3f(sliceZ + offset, x0, y1);
            } else if (currentAxis == SliceAxis::Y) {
                glVertex3f(x0, sliceZ + offset, y0);
                glVertex3f(x1, sliceZ + offset, y0);
                glVertex3f(x1, sliceZ + offset, y1);
                glVertex3f(x0, sliceZ + offset, y1);
            } else {
                glVertex3f(x0, y0, sliceZ + offset);
                glVertex3f(x1, y0, sliceZ + offset);
                glVertex3f(x1, y1, sliceZ + offset);
                glVertex3f(x0, y1, sliceZ + offset);
            }
            glEnd();
        }
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    
    if (showWireframe || showSliceGrid) {
        glLineWidth(2.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
        
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, currentLayerData.triangles.size() * 3, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
    }
    glEnable(GL_LIGHTING);
}

void MeshSlicerVis::keyPressEvent(QKeyEvent* e) {
    bool shifted = e->modifiers() & Qt::ShiftModifier;

    switch (e->key()) {
        // --- Layer Navigation ---
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Space:
            currentLayerIdx = (currentLayerIdx + 1) % numLayers;
            updateCurrentLayer();
            break;
        case Qt::Key_Left:
        case Qt::Key_Down:
            currentLayerIdx = (currentLayerIdx - 1 + numLayers) % numLayers;
            updateCurrentLayer();
            break;
        case Qt::Key_PageUp:
            currentLayerIdx = std::min(currentLayerIdx + std::max(1, numLayers / 10), numLayers - 1);
            updateCurrentLayer();
            break;
        case Qt::Key_PageDown:
            currentLayerIdx = std::max(currentLayerIdx - std::max(1, numLayers / 10), 0);
            updateCurrentLayer();
            break;
        case Qt::Key_Home:
            currentLayerIdx = 0;
            updateCurrentLayer();
            break;
        case Qt::Key_End:
            currentLayerIdx = numLayers - 1;
            updateCurrentLayer();
            break;

        // --- Resolution & Layer Count ---
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            resolution = std::min(resolution + 32, 2048);
            updateCurrentLayer();
            break;
        case Qt::Key_Minus:
            resolution = std::max(resolution - 32, 32);
            updateCurrentLayer();
            break;
        case Qt::Key_1: resolution = 64;   updateCurrentLayer(); break;
        case Qt::Key_2: resolution = 128;  updateCurrentLayer(); break;
        case Qt::Key_3: resolution = 256;  updateCurrentLayer(); break;
        case Qt::Key_4: resolution = 384;  updateCurrentLayer(); break;
        case Qt::Key_5: resolution = 512;  updateCurrentLayer(); break;
        case Qt::Key_6: resolution = 768;  updateCurrentLayer(); break;
        case Qt::Key_7: resolution = 1024; updateCurrentLayer(); break;
        case Qt::Key_8: resolution = 1536; updateCurrentLayer(); break;
        case Qt::Key_9: resolution = 2048; updateCurrentLayer(); break;

        case Qt::Key_BracketRight:
            numLayers = std::min(numLayers + 10, 1000);
            updateCurrentLayer();
            break;
        case Qt::Key_BracketLeft:
            numLayers = std::max(numLayers - 10, 10);
            currentLayerIdx = std::min(currentLayerIdx, numLayers - 1);
            updateCurrentLayer();
            break;

        // --- Toggles ---
        case Qt::Key_W:
            showWireframe = !showWireframe;
            update();
            break;
        case Qt::Key_G:
            showSliceGrid = !showSliceGrid;
            update();
            std::cout << "Slice Grid: " << (showSliceGrid ? "ON" : "OFF") << std::endl;
            break;
        case Qt::Key_F:
            showFilled = !showFilled;
            update();
            std::cout << "Slice: " << (showFilled ? "ON" : "OFF") << std::endl;
            break;
        case Qt::Key_T:
            meshVisible = !meshVisible;
            update();
            std::cout << "Mesh: " << (meshVisible ? "VISIBLE" : "HIDDEN") << std::endl;
            break;
        case Qt::Key_B:
            showBBox = !showBBox;
            update();
            std::cout << "Bounding Box: " << (showBBox ? "VISIBLE" : "HIDDEN") << std::endl;
            break;
        case Qt::Key_D:
            showRays = !showRays;
            update();
            std::cout << "Rays: " << (showRays ? "VISIBLE" : "HIDDEN") << std::endl;
            break;
        case Qt::Key_E:
            showRaySources = !showRaySources;
            update();
            std::cout << "Ray Sources: " << (showRaySources ? "VISIBLE" : "HIDDEN") << std::endl;
            break;

        // --- Axis ---
        case Qt::Key_X:
            currentAxis = SliceAxis::X;
            currentLayerIdx = 0;
            updateCurrentLayer();
            std::cout << "Direction: X-axis" << std::endl;
            break;
        case Qt::Key_Y:
            currentAxis = SliceAxis::Y;
            currentLayerIdx = 0;
            updateCurrentLayer();
            std::cout << "Direction: Y-axis" << std::endl;
            break;
        case Qt::Key_Z:
            currentAxis = SliceAxis::Z;
            currentLayerIdx = 0;
            updateCurrentLayer();
            std::cout << "Direction: Z-axis" << std::endl;
            break;

        // --- Utility ---
        case Qt::Key_R:
            camera()->showEntireScene();
            update();
            break;
        case Qt::Key_H:
            init(); // Re-print help
            break;
        case Qt::Key_O:
            if (camera()->type() == qglviewer::Camera::PERSPECTIVE) {
                camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
                std::cout << "Camera: ORTHOGRAPHIC" << std::endl;
            } else {
                camera()->setType(qglviewer::Camera::PERSPECTIVE);
                std::cout << "Camera: PERSPECTIVE" << std::endl;
            }
            update();
            break;
        case Qt::Key_S:
            saveSlice();
            break;

        default:
            QGLViewer::keyPressEvent(e);
            break;
    }
}

void MeshSlicerVis::saveSlice() {
    std::string filename = "slice_" + std::to_string(currentLayerIdx) + ".ppm";
    std::ofstream out(filename, std::ios::binary);
    if (out) {
        out << "P6\n" << currentLayerData.texWidth << " " << currentLayerData.texHeight << "\n255\n";
        out.write((char*)currentLayerData.textureData.data(), currentLayerData.textureData.size());
        std::cout << "Saved slice to " << filename << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2 || (argc >= 2 && std::string(argv[1]) == "-h")) {
        std::cerr << "Usage: " << argv[0] << " <mesh_file> [num_layers] [resolution]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  <mesh_file>    Input mesh file (OFF/PLY format)" << std::endl;
        std::cerr << "  [num_layers]   Number of slicing layers (default: 50)" << std::endl;
        std::cerr << "  [resolution]  Slicing resolution (default: 128)" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    int numLayers = (argc > 2) ? std::stoi(argv[2]) : 50;
    int resolution = (argc > 3) ? std::stoi(argv[3]) : 128;
    
    QApplication app(argc, argv);
    app.setApplicationName("Mesh Slicer Viewer");
    
    MeshSlicerVis viewer(inputFile, numLayers, resolution);
    viewer.setWindowTitle("Mesh Slicer Viewer");
    viewer.resize(1024, 768);
    viewer.show();
    
    return app.exec();
}
