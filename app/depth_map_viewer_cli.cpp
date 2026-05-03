#include "RayTracer.h"
#include "MeshIO.h"
#include "ShapeDiameter.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <memory>
#include <tbb/parallel_for.h>

#include <QApplication>
#include <QGLViewer/qglviewer.h>
#include <QKeyEvent>

class DepthMapVis : public QGLViewer {
public:
    DepthMapVis(const std::string& filename, bool startWithTextures = true) 
        : inputFile(filename), showTextures(startWithTextures) {
        for(int i=0; i<6; ++i) textures[i] = 0;
    }
    ~DepthMapVis() {
        glDeleteTextures(6, textures);
        if (vbo_vertices) glDeleteBuffers(1, &vbo_vertices);
        if (vbo_indices) glDeleteBuffers(1, &vbo_indices);
        if (vbo_normals) glDeleteBuffers(1, &vbo_normals);
    }

protected:
    virtual void draw() override;
    virtual void init() override;
    virtual void keyPressEvent(QKeyEvent* e) override;

private:
    std::string inputFile;
    Mesh mesh;
    AABB box;
    std::unique_ptr<ShapeDiameter> sd;
    GLuint textures[6];
    int res = 512;
    bool showBox = true;
    bool showTextures = true;
    bool showWireframe = false;
    bool showLighting = true;
    bool showSmoothShading = true;

    GLuint vbo_vertices = 0;
    GLuint vbo_indices = 0;
    GLuint vbo_normals = 0;

    void loadMesh();
    void computeDepthMaps();
    void createTexture(int idx, const std::vector<unsigned char>& data);
    void drawBoxPlanes();
    void drawBoxWireframe();
    void drawMesh();
    void setupVBOs();
};

void DepthMapVis::loadMesh() {
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        exit(1);
    }
    for (const auto& v : mesh.vertices) box.expand(v);
    box.pad(0.01f);
}

void DepthMapVis::setupVBOs() {
    if(vbo_vertices) glDeleteBuffers(1, &vbo_vertices);
    if(vbo_indices) glDeleteBuffers(1, &vbo_indices);
    if(vbo_normals) glDeleteBuffers(1, &vbo_normals);

    glGenBuffers(1, &vbo_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    std::vector<Vec3> normals(mesh.vertices.size(), {0, 0, 0});
    for (const auto& t : mesh.triangles) {
        Vec3 n = computeFaceNormal(mesh.vertices[t.v0], mesh.vertices[t.v1], mesh.vertices[t.v2]);
        normals[t.v0].x += n.x; normals[t.v0].y += n.y; normals[t.v0].z += n.z;
        normals[t.v1].x += n.x; normals[t.v1].y += n.y; normals[t.v1].z += n.z;
        normals[t.v2].x += n.x; normals[t.v2].y += n.y; normals[t.v2].z += n.z;
    }
    for (auto& n : normals) {
        float len = n.length();
        if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    }

    glGenBuffers(1, &vbo_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(Vec3), normals.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &vbo_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.triangles.size() * sizeof(Triangle), mesh.triangles.data(), GL_STATIC_DRAW);
}

void DepthMapVis::createTexture(int idx, const std::vector<unsigned char>& data) {
    if (textures[idx]) glDeleteTextures(1, &textures[idx]);
    glGenTextures(1, &textures[idx]);
    glBindTexture(GL_TEXTURE_2D, textures[idx]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, res, res, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void DepthMapVis::computeDepthMaps() {
    Vec3 size = box.size();
    for (int side_idx = 0; side_idx < 6; ++side_idx) {
        int axis = side_idx / 2; int side = side_idx % 2;
        float u_size, v_size, w_size;
        Vec3 org, dir;
        if (axis == 0) { u_size = size.z; v_size = size.y; w_size = size.x; dir = (side == 0) ? Vec3{1, 0, 0} : Vec3{-1, 0, 0}; org.x = (side == 0) ? box.min.x : box.max.x; }
        else if (axis == 1) { u_size = size.x; v_size = size.z; w_size = size.y; dir = (side == 0) ? Vec3{0, 1, 0} : Vec3{0, -1, 0}; org.y = (side == 0) ? box.min.y : box.max.y; }
        else { u_size = size.x; v_size = size.y; w_size = size.z; dir = (side == 0) ? Vec3{0, 0, 1} : Vec3{0, 0, -1}; org.z = (side == 0) ? box.min.z : box.max.z; }

        std::vector<unsigned char> data(res * res * 3);
        tbb::parallel_for(0, res * res, [&](int p) {
            int iu = p % res; int iv = p / res;
            float u = (iu + 0.5f) / res; float v = (iv + 0.5f) / res;
            Ray ray;
            if (axis == 0) { ray.org = {org.x, box.min.y + v * v_size, box.min.z + u * u_size}; }
            else if (axis == 1) { ray.org = {box.min.x + u * u_size, org.y, box.min.z + v * v_size}; }
            else { ray.org = {box.min.x + u * u_size, box.min.y + v * v_size, org.z}; }
            ray.dir = dir;
            ray.tnear = 0.0f; ray.tfar = w_size * 1.1f;
            
            Hit hit = RayTracer::intersect(sd->getScene(), ray);
            Color4b color = {0, 0, 0, 255};
            if (hit.hit) color = getJetColor(1.0f - (hit.t / (w_size + 1e-6f)));
            data[p * 3 + 0] = color.r;
            data[p * 3 + 1] = color.g;
            data[p * 3 + 2] = color.b;
        });
        createTexture(side_idx, data);
    }
}

void DepthMapVis::init() {
    loadMesh();
    sd = std::make_unique<ShapeDiameter>(mesh);
    computeDepthMaps(); setupVBOs();
    Vec3 c = {(box.min.x + box.max.x)/2.0f, (box.min.y + box.max.y)/2.0f, (box.min.z + box.max.z)/2.0f};
    setSceneCenter(qglviewer::Vec(c.x, c.y, c.z));
    setSceneRadius(box.size().length()/2.0f);
    showEntireScene(); glEnable(GL_DEPTH_TEST);
}

void DepthMapVis::drawMesh() {
    if (showLighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    glShadeModel(showSmoothShading ? GL_SMOOTH : GL_FLAT);
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK);
    glColor3f(0.8f, 0.8f, 0.8f);
    glPolygonMode(GL_FRONT_AND_BACK, showWireframe ? GL_LINE : GL_FILL);
    glEnableClientState(GL_VERTEX_ARRAY);
    if (showSmoothShading) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normals); glNormalPointer(GL_FLOAT, sizeof(Vec3), 0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices); glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices); glDrawElements(GL_TRIANGLES, mesh.triangles.size() * 3, GL_UNSIGNED_INT, 0);
    if (showSmoothShading) glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
}

void DepthMapVis::drawBoxPlanes() {
    if (!showTextures) return;
    glDisable(GL_LIGHTING); glEnable(GL_TEXTURE_2D); 
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glBindTexture(GL_TEXTURE_2D, textures[0]); glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.min.z); glTexCoord2f(1,0); glVertex3f(box.min.x, box.min.y, box.max.z); glTexCoord2f(1,1); glVertex3f(box.min.x, box.max.y, box.max.z); glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.min.z); glEnd();
    glBindTexture(GL_TEXTURE_2D, textures[1]); glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(box.max.x, box.min.y, box.min.z); glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.max.z); glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.max.z); glTexCoord2f(0,1); glVertex3f(box.max.x, box.max.y, box.min.z); glEnd();
    glBindTexture(GL_TEXTURE_2D, textures[2]); glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.min.z); glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.min.z); glTexCoord2f(1,1); glVertex3f(box.max.x, box.min.y, box.max.z); glTexCoord2f(0,1); glVertex3f(box.min.x, box.min.y, box.max.z); glEnd();
    glBindTexture(GL_TEXTURE_2D, textures[3]); glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(box.min.x, box.max.y, box.min.z); glTexCoord2f(1,0); glVertex3f(box.max.x, box.max.y, box.min.z); glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.max.z); glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.max.z); glEnd();
    glBindTexture(GL_TEXTURE_2D, textures[4]); glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.min.z); glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.min.z); glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.min.z); glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.min.z); glEnd();
    glBindTexture(GL_TEXTURE_2D, textures[5]); glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.max.z); glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.max.z); glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.max.z); glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.max.z); glEnd();
    glDisable(GL_TEXTURE_2D);
}

void DepthMapVis::drawBoxWireframe() {
    glDisable(GL_LIGHTING); glLineWidth(2.0f); glColor3f(1.0f, 1.0f, 1.0f); glBegin(GL_LINES);
    glVertex3f(box.min.x, box.min.y, box.min.z); glVertex3f(box.max.x, box.min.y, box.min.z);
    glVertex3f(box.max.x, box.min.y, box.min.z); glVertex3f(box.max.x, box.min.y, box.max.z);
    glVertex3f(box.max.x, box.min.y, box.max.z); glVertex3f(box.min.x, box.min.y, box.max.z);
    glVertex3f(box.min.x, box.min.y, box.max.z); glVertex3f(box.min.x, box.min.y, box.min.z);
    glVertex3f(box.min.x, box.max.y, box.min.z); glVertex3f(box.max.x, box.max.y, box.min.z);
    glVertex3f(box.max.x, box.max.y, box.min.z); glVertex3f(box.max.x, box.max.y, box.max.z);
    glVertex3f(box.max.x, box.max.y, box.max.z); glVertex3f(box.min.x, box.max.y, box.max.z);
    glVertex3f(box.min.x, box.max.y, box.max.z); glVertex3f(box.min.x, box.max.y, box.min.z);
    glVertex3f(box.min.x, box.min.y, box.min.z); glVertex3f(box.min.x, box.max.y, box.min.z);
    glVertex3f(box.max.x, box.min.y, box.min.z); glVertex3f(box.max.x, box.max.y, box.min.z);
    glVertex3f(box.max.x, box.min.y, box.max.z); glVertex3f(box.max.x, box.max.y, box.max.z);
    glVertex3f(box.min.x, box.min.y, box.max.z); glVertex3f(box.min.x, box.max.y, box.max.z);
    glEnd(); glLineWidth(1.0f);
}

void DepthMapVis::draw() {
    drawMesh();
    if (showBox) { 
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawBoxPlanes(); drawBoxWireframe();
        glDisable(GL_BLEND);
    }
}

void DepthMapVis::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) exit(0);
    else if (e->key() == Qt::Key_B) { showBox = !showBox; update(); }
    else if (e->key() == Qt::Key_T) { showTextures = !showTextures; update(); }
    else if (e->key() == Qt::Key_W) { showWireframe = !showWireframe; update(); }
    else if (e->key() == Qt::Key_L) { showLighting = !showLighting; update(); }
    else if (e->key() == Qt::Key_S) { showSmoothShading = !showSmoothShading; update(); }
    else QGLViewer::keyPressEvent(e);
}

int main(int argc, char** argv) {
    std::string modelFile = "";
    bool showTextures = true;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t") showTextures = false;
        else modelFile = arg;
    }
    if (modelFile.empty()) return 1;
    QApplication app(argc, argv);
    DepthMapVis viewer(modelFile, showTextures);
    viewer.resize(1000, 1000);
    viewer.show();
    return app.exec();
}
