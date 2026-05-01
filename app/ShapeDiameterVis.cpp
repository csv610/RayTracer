#include <QApplication>
#include <QGLViewer/qglviewer.h>
#include <QKeyEvent>
#include <embree4/rtcore.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh_utils.h"
#include "ShapeDiameter.h"

class ShapeDiameterVis : public QGLViewer {
public:
    ShapeDiameterVis(const std::string& filename) : inputFile(filename) {}

protected:
    virtual void draw() override;
    virtual void init() override;
    virtual void keyPressEvent(QKeyEvent* e) override;
    virtual void postSelection(const QPoint& point) override;

private:
    std::string inputFile;
    Mesh mesh;
    
    ShapeDiameter* sd = nullptr;
    
    int selectedTriangle = -1;
    bool showWireframe = false;
    std::vector<int> hitTriangles;
    std::vector<ShapeDiameter::RayHit> currentHits;

    void loadMesh();
    void shootRays(int triIdx);
    void drawMesh();
    void drawRays();
};

void ShapeDiameterVis::loadMesh() {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(inputFile, 
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        exit(1);
    }

    mesh.vertices.clear();
    mesh.triangles.clear();

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* aiM = scene->mMeshes[i];
        unsigned int offset = mesh.vertices.size();

        for (unsigned int j = 0; j < aiM->mNumVertices; ++j) {
            mesh.vertices.push_back({aiM->mVertices[j].x, aiM->mVertices[j].y, aiM->mVertices[j].z});
        }

        for (unsigned int j = 0; j < aiM->mNumFaces; ++j) {
            aiFace face = aiM->mFaces[j];
            if (face.mNumIndices == 3) {
                mesh.triangles.push_back({face.mIndices[0] + offset, face.mIndices[1] + offset, face.mIndices[2] + offset});
            }
        }
    }
    std::cout << "Mesh loaded with Assimp: " << mesh.vertices.size() << " vertices, " << mesh.triangles.size() << " triangles" << std::endl;
}

void ShapeDiameterVis::init() {
    loadMesh();
    sd = new ShapeDiameter(mesh);
    
    // Set scene center and radius for rotation and camera fit
    if (!mesh.vertices.empty()) {
        Vec3 minP = {mesh.vertices[0].x, mesh.vertices[0].y, mesh.vertices[0].z};
        Vec3 maxP = {mesh.vertices[0].x, mesh.vertices[0].y, mesh.vertices[0].z};
        for (const auto& v : mesh.vertices) {
            minP.x = std::min(minP.x, v.x); minP.y = std::min(minP.y, v.y); minP.z = std::min(minP.z, v.z);
            maxP.x = std::max(maxP.x, v.x); maxP.y = std::max(maxP.y, v.y); maxP.z = std::max(maxP.z, v.z);
        }
        Vec3 center = {(minP.x + maxP.x)/2.0f, (minP.y + maxP.y)/2.0f, (minP.z + maxP.z)/2.0f};
        float radius = sqrtf(powf(maxP.x - minP.x, 2) + powf(maxP.y - minP.y, 2) + powf(maxP.z - minP.z, 2)) / 2.0f;
        
        setSceneCenter(qglviewer::Vec(center.x, center.y, center.z));
        setSceneRadius(radius);
        showEntireScene();
    }

    restoreStateFromFile();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}

void ShapeDiameterVis::drawMesh() {
    // Pass 1: Draw regular triangles (not selected or hit)
    glPolygonMode(GL_FRONT_AND_BACK, showWireframe ? GL_LINE : GL_FILL);
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        bool isHit = std::find(hitTriangles.begin(), hitTriangles.end(), (int)i) != hitTriangles.end();
        if ((int)i != selectedTriangle && !isHit) {
            glColor3f(0.8f, 0.8f, 0.8f); // Default: Light Gray
            const Triangle& t = mesh.triangles[i];
            const Vertex& v0 = mesh.vertices[t.v0];
            const Vertex& v1 = mesh.vertices[t.v1];
            const Vertex& v2 = mesh.vertices[t.v2];
            Vec3 n = computeFaceNormal(v0, v1, v2);
            glNormal3f(n.x, n.y, n.z);
            glVertex3f(v0.x, v0.y, v0.z);
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v2.x, v2.y, v2.z);
        }
    }
    glEnd();

    // Pass 2: Draw selected and hit triangles as solid
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_TRIANGLES);
    // Draw hit triangles in Red
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        bool isHit = std::find(hitTriangles.begin(), hitTriangles.end(), (int)i) != hitTriangles.end();
        if (isHit && (int)i != selectedTriangle) {
            glColor3f(1.0f, 0.0f, 0.0f);
            const Triangle& t = mesh.triangles[i];
            const Vertex& v0 = mesh.vertices[t.v0];
            const Vertex& v1 = mesh.vertices[t.v1];
            const Vertex& v2 = mesh.vertices[t.v2];
            Vec3 n = computeFaceNormal(v0, v1, v2);
            glNormal3f(n.x, n.y, n.z);
            glVertex3f(v0.x, v0.y, v0.z);
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v2.x, v2.y, v2.z);
        }
    }
    // Draw selected triangle in Green
    if (selectedTriangle != -1) {
        glColor3f(0.0f, 1.0f, 0.0f);
        const Triangle& t = mesh.triangles[selectedTriangle];
        const Vertex& v0 = mesh.vertices[t.v0];
        const Vertex& v1 = mesh.vertices[t.v1];
        const Vertex& v2 = mesh.vertices[t.v2];
        Vec3 n = computeFaceNormal(v0, v1, v2);
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(v0.x, v0.y, v0.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();
}

void ShapeDiameterVis::drawRays() {
    if (selectedTriangle == -1) return;
    Vec3 faceCenter = computeFaceCenter(mesh.vertices[mesh.triangles[selectedTriangle].v0], 
                                         mesh.vertices[mesh.triangles[selectedTriangle].v1], 
                                         mesh.vertices[mesh.triangles[selectedTriangle].v2]);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f); // Rays: Yellow
    glBegin(GL_LINES);
    for (const auto& h : currentHits) {
        glVertex3f(faceCenter.x, faceCenter.y, faceCenter.z);
        glVertex3f(faceCenter.x + h.dir.x * h.distance,
                   faceCenter.y + h.dir.y * h.distance,
                   faceCenter.z + h.dir.z * h.distance);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void ShapeDiameterVis::draw() {
    drawMesh();
    drawRays();
}

void ShapeDiameterVis::shootRays(int triIdx) {
    hitTriangles.clear();
    currentHits.clear();
    
    if (sd && triIdx >= 0) {
        sd->computeForFace(triIdx, currentHits);
        for (const auto& h : currentHits) {
            hitTriangles.push_back(h.primID);
        }
    }
    update();
}

void ShapeDiameterVis::postSelection(const QPoint& point) {
    bool found;
    qglviewer::Vec selectedPoint = camera()->pointUnderPixel(point, found);
    if (found) {
        float minDist = 1e20f;
        int bestIdx = -1;
        Vec3 p = {(float)selectedPoint.x, (float)selectedPoint.y, (float)selectedPoint.z};
        for (size_t i = 0; i < mesh.triangles.size(); ++i) {
            Vec3 center = computeFaceCenter(mesh.vertices[mesh.triangles[i].v0], mesh.vertices[mesh.triangles[i].v1], mesh.vertices[mesh.triangles[i].v2]);
            float d = (center.x-p.x)*(center.x-p.x) + (center.y-p.y)*(center.y-p.y) + (center.z-p.z)*(center.z-p.z);
            if (d < minDist) { minDist = d; bestIdx = i; }
        }
        if (bestIdx != -1) {
            selectedTriangle = bestIdx;
            shootRays(selectedTriangle);
        }
    }
}

void ShapeDiameterVis::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) exit(0);
    else if (e->key() == Qt::Key_W) {
        showWireframe = !showWireframe;
        update();
    }
    else QGLViewer::keyPressEvent(e);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <mesh.off>" << std::endl;
        return 1;
    }
    QApplication application(argc, argv);
    ShapeDiameterVis viewer(argv[1]);
    viewer.show();
    return application.exec();
}
