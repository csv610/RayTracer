#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <execution>
#include <numeric>
#include "mesh_utils.h"
#include "MeshIO.h"
#include "ShapeDiameter.h"

#include <QApplication>
#include <QGLViewer/qglviewer.h>
#include <QKeyEvent>

class ShapeDiameterVis : public QGLViewer {
public:
    ShapeDiameterVis(const std::string& filename) : inputFile(filename) {}
    ~ShapeDiameterVis() {
        if (vbo_vertices) glDeleteBuffers(1, &vbo_vertices);
        if (vbo_indices) glDeleteBuffers(1, &vbo_indices);
        if (vbo_normals) glDeleteBuffers(1, &vbo_normals);
    }

protected:
    virtual void draw() override;
    virtual void init() override;
    virtual void keyPressEvent(QKeyEvent* e) override;
    virtual void postSelection(const QPoint& point) override;

private:
    std::string inputFile;
    Mesh mesh;
    std::unique_ptr<ShapeDiameter> sd;
    
    int selectedTriangle = -1;
    bool showWireframe = false;
    bool showLighting = true;
    bool showSmoothShading = true;

    std::vector<int> hitTriangles;
    std::vector<ShapeDiameter::RayHit> currentHits;

    GLuint vbo_vertices = 0;
    GLuint vbo_indices = 0;
    GLuint vbo_normals = 0;

    void loadMesh();
    void shootRays(int triIdx);
    void drawMesh();
    void drawRays();
    void setupVBOs();
};

void ShapeDiameterVis::loadMesh() {
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        exit(1);
    }
}

void ShapeDiameterVis::setupVBOs() {
    if(vbo_vertices) glDeleteBuffers(1, &vbo_vertices);
    if(vbo_indices) glDeleteBuffers(1, &vbo_indices);
    if(vbo_normals) glDeleteBuffers(1, &vbo_normals);

    glGenBuffers(1, &vbo_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    // Compute Vertex Normals
    std::vector<Vec3> normals(mesh.vertices.size(), {0, 0, 0});
    for (const auto& t : mesh.triangles) {
        Vec3 n = computeFaceNormal(mesh.vertices[t.v0], mesh.vertices[t.v1], mesh.vertices[t.v2]);
        normals[t.v0].x += n.x; normals[t.v0].y += n.y; normals[t.v0].z += n.z;
        normals[t.v1].x += n.x; normals[t.v1].y += n.y; normals[t.v1].z += n.z;
        normals[t.v2].x += n.x; normals[t.v2].y += n.y; normals[t.v2].z += n.z;
    }
    for (auto& n : normals) {
        float len = sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
        if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    }
    glGenBuffers(1, &vbo_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(Vec3), normals.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &vbo_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.triangles.size() * sizeof(Triangle), mesh.triangles.data(), GL_STATIC_DRAW);
}

void ShapeDiameterVis::init() {
    loadMesh();
    sd = std::make_unique<ShapeDiameter>(mesh);
    setupVBOs();
    if (!mesh.vertices.empty()) {
        Vec3 minP = {mesh.vertices[0].x, mesh.vertices[0].y, mesh.vertices[0].z};
        Vec3 maxP = minP;
        for (const auto& v : mesh.vertices) {
            minP.x = std::min(minP.x, v.x); minP.y = std::min(minP.y, v.y); minP.z = std::min(minP.z, v.z);
            maxP.x = std::max(maxP.x, v.x); maxP.y = std::max(maxP.y, v.y); maxP.z = std::max(maxP.z, v.z);
        }
        Vec3 center = {(minP.x + maxP.x)/2.0f, (minP.y + maxP.y)/2.0f, (minP.z + maxP.z)/2.0f};
        float radius = sqrtf(powf(maxP.x-minP.x,2)+powf(maxP.y-minP.y,2)+powf(maxP.z-minP.z,2))/2.0f;
        setSceneCenter(qglviewer::Vec(center.x, center.y, center.z));
        setSceneRadius(radius);
        showEntireScene();
    }
    glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
}

void ShapeDiameterVis::drawMesh() {
    if (showLighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    glShadeModel(showSmoothShading ? GL_SMOOTH : GL_FLAT);
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK);

    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices); glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
    
    if (showSmoothShading) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normals); glNormalPointer(GL_FLOAT, sizeof(Vec3), 0);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);

    // Pass 1: Background mesh
    glPolygonMode(GL_FRONT_AND_BACK, showWireframe ? GL_LINE : GL_FILL);
    glColor3f(0.8f, 0.8f, 0.8f);
    glDrawElements(GL_TRIANGLES, mesh.triangles.size() * 3, GL_UNSIGNED_INT, 0);

    // Pass 2: Highlights
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_LIGHTING); // Highights usually unlit for clarity
    
    glColor3f(1, 0, 0);
    glBegin(GL_TRIANGLES);
    for (int id : hitTriangles) {
        if (id == selectedTriangle) continue;
        const auto& t = mesh.triangles[id];
        glVertex3fv(&mesh.vertices[t.v0].x); glVertex3fv(&mesh.vertices[t.v1].x); glVertex3fv(&mesh.vertices[t.v2].x);
    }
    glEnd();

    if (selectedTriangle != -1) {
        glColor3f(0, 1, 0);
        glBegin(GL_TRIANGLES);
        const auto& t = mesh.triangles[selectedTriangle];
        glVertex3fv(&mesh.vertices[t.v0].x); glVertex3fv(&mesh.vertices[t.v1].x); glVertex3fv(&mesh.vertices[t.v2].x);
        glEnd();
    }

    if (showSmoothShading) glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
}

void ShapeDiameterVis::drawRays() {
    if (selectedTriangle == -1) return;
    Vec3 faceCenter = computeFaceCenter(mesh.vertices[mesh.triangles[selectedTriangle].v0], mesh.vertices[mesh.triangles[selectedTriangle].v1], mesh.vertices[mesh.triangles[selectedTriangle].v2]);
    glDisable(GL_LIGHTING); glColor3f(1, 1, 0); glBegin(GL_LINES);
    for (const auto& h : currentHits) {
        glVertex3f(faceCenter.x, faceCenter.y, faceCenter.z);
        glVertex3f(faceCenter.x + h.dir.x * h.distance, faceCenter.y + h.dir.y * h.distance, faceCenter.z + h.dir.z * h.distance);
    }
    glEnd();
    if (showLighting) glEnable(GL_LIGHTING);
}

void ShapeDiameterVis::draw() { drawMesh(); drawRays(); }

void ShapeDiameterVis::shootRays(int triIdx) {
    hitTriangles.clear(); currentHits.clear();
    if (sd && triIdx >= 0) {
        sd->computeForFace(triIdx, currentHits);
        for (const auto& h : currentHits) hitTriangles.push_back(h.primID);
    }
    update();
}

void ShapeDiameterVis::postSelection(const QPoint& point) {
    bool found; qglviewer::Vec selectedPoint = camera()->pointUnderPixel(point, found);
    if (found) {
        float minDist = 1e20f; int bestIdx = -1; Vec3 p = {(float)selectedPoint.x, (float)selectedPoint.y, (float)selectedPoint.z};
        for (size_t i = 0; i < mesh.triangles.size(); ++i) {
            Vec3 center = computeFaceCenter(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
            float d = (center.x-p.x)*(center.x-p.x) + (center.y-p.y)*(center.y-p.y) + (center.z-p.z)*(center.z-p.z);
            if (d < minDist) { minDist = d; bestIdx = (int)i; }
        }
        if (bestIdx != -1) { selectedTriangle = bestIdx; shootRays(selectedTriangle); }
    }
}

void ShapeDiameterVis::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) exit(0);
    else if (e->key() == Qt::Key_W) { showWireframe = !showWireframe; update(); }
    else if (e->key() == Qt::Key_L) { showLighting = !showLighting; update(); }
    else if (e->key() == Qt::Key_S) { showSmoothShading = !showSmoothShading; update(); }
    else QGLViewer::keyPressEvent(e);
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cout << "Usage: " << argv[0] << " <mesh.off>" << std::endl; return 1; }
    QApplication application(argc, argv);
    ShapeDiameterVis viewer(argv[1]);
    viewer.resize(1000, 1000);
    viewer.show();
    return application.exec();
}
