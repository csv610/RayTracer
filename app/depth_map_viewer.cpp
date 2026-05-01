#include <QApplication>
#include <QGLViewer/qglviewer.h>
#include <QKeyEvent>
#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh_utils.h"

struct AABB {
    Vec3 min = {1e20f, 1e20f, 1e20f};
    Vec3 max = {-1e20f, -1e20f, -1e20f};
    void expand(const Vertex& v) {
        min.x = std::min(min.x, v.x); min.y = std::min(min.y, v.y); min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x); max.y = std::max(max.y, v.y); max.z = std::max(max.z, v.z);
    }
    Vec3 size() const { return {max.x - min.x, max.y - min.y, max.z - min.z}; }
};

class DepthMapVis : public QGLViewer {
public:
    DepthMapVis(const std::string& filename) : inputFile(filename) {}

protected:
    virtual void draw() override;
    virtual void init() override;
    virtual void keyPressEvent(QKeyEvent* e) override;

private:
    std::string inputFile;
    Mesh mesh;
    AABB box;
    RTCDevice device = nullptr;
    RTCScene scene = nullptr;
    
    GLuint textures[6]; // Xmin, Xmax, Ymin, Ymax, Zmin, Zmax
    int res = 512;
    bool showBox = true;

    void loadMesh();
    void buildEmbreeScene();
    void computeDepthMaps();
    void createTexture(int idx, const std::vector<unsigned char>& data);
    void drawBoxPlanes();
    void drawMesh();
};

void DepthMapVis::loadMesh() {
    Assimp::Importer importer;
    const aiScene* aiS = importer.ReadFile(inputFile, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
    if (!aiS || !aiS->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        exit(1);
    }
    for (unsigned int i = 0; i < aiS->mNumMeshes; ++i) {
        aiMesh* m = aiS->mMeshes[i];
        unsigned int offset = mesh.vertices.size();
        for (unsigned int j = 0; j < m->mNumVertices; ++j) {
            Vertex v = {m->mVertices[j].x, m->mVertices[j].y, m->mVertices[j].z};
            mesh.vertices.push_back(v);
            box.expand(v);
        }
        for (unsigned int j = 0; j < m->mNumFaces; ++j) {
            if (m->mFaces[j].mNumIndices == 3)
                mesh.triangles.push_back({m->mFaces[j].mIndices[0] + offset, m->mFaces[j].mIndices[1] + offset, m->mFaces[j].mIndices[2] + offset});
        }
    }
}

void DepthMapVis::buildEmbreeScene() {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene);
}

void DepthMapVis::createTexture(int idx, const std::vector<unsigned char>& data) {
    glGenTextures(1, &textures[idx]);
    glBindTexture(GL_TEXTURE_2D, textures[idx]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, res, res, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void DepthMapVis::computeDepthMaps() {
    Vec3 size = box.size();
    for (int side_idx = 0; side_idx < 6; ++side_idx) {
        int axis = side_idx / 2;
        int side = side_idx % 2;
        
        float u_size, v_size, w_size;
        Vec3 org, dir;
        if (axis == 0) { // X
            u_size = size.z; v_size = size.y; w_size = size.x;
            dir = (side == 0) ? Vec3{1, 0, 0} : Vec3{-1, 0, 0};
            org.x = (side == 0) ? box.min.x : box.max.x;
        } else if (axis == 1) { // Y
            u_size = size.x; v_size = size.z; w_size = size.y;
            dir = (side == 0) ? Vec3{0, 1, 0} : Vec3{0, -1, 0};
            org.y = (side == 0) ? box.min.y : box.max.y;
        } else { // Z
            u_size = size.x; v_size = size.y; w_size = size.z;
            dir = (side == 0) ? Vec3{0, 0, 1} : Vec3{0, 0, -1};
            org.z = (side == 0) ? box.min.z : box.max.z;
        }

        std::vector<unsigned char> data(res * res * 3);
        for (int iv = 0; iv < res; ++iv) {
            for (int iu = 0; iu < res; ++iu) {
                float u = (iu + 0.5f) / res;
                float v = (iv + 0.5f) / res;
                RTCRayHit rh;
                if (axis == 0) {
                    rh.ray.org_x = org.x; rh.ray.org_y = box.min.y + v * v_size; rh.ray.org_z = box.min.z + u * u_size;
                } else if (axis == 1) {
                    rh.ray.org_x = box.min.x + u * u_size; rh.ray.org_y = org.y; rh.ray.org_z = box.min.z + v * v_size;
                } else {
                    rh.ray.org_x = box.min.x + u * u_size; rh.ray.org_y = box.min.y + v * v_size; rh.ray.org_z = org.z;
                }
                rh.ray.dir_x = dir.x; rh.ray.dir_y = dir.y; rh.ray.dir_z = dir.z;
                rh.ray.tnear = 0.0f; rh.ray.tfar = w_size * 1.1f;
                rh.ray.mask = -1; rh.ray.time = 0; rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                RTCIntersectArguments args; rtcInitIntersectArguments(&args);
                rtcIntersect1(scene, &rh, &args);

                Vec3 color = {0, 0, 0};
                if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                    float val = rh.ray.tfar / (w_size + 1e-6f);
                    color = getJetColor(1.0f - val);
                }
                int base = (iv * res + iu) * 3;
                data[base + 0] = (unsigned char)(color.x * 255);
                data[base + 1] = (unsigned char)(color.y * 255);
                data[base + 2] = (unsigned char)(color.z * 255);
            }
        }
        createTexture(side_idx, data);
    }
}

void DepthMapVis::init() {
    loadMesh();
    buildEmbreeScene();
    computeDepthMaps();
    
    Vec3 c = {(box.min.x + box.max.x)/2.0f, (box.min.y + box.max.y)/2.0f, (box.min.z + box.max.z)/2.0f};
    float r = sqrtf(powf(box.max.x-box.min.x, 2) + powf(box.max.y-box.min.y, 2) + powf(box.max.z-box.min.z, 2))/2.0f;
    setSceneCenter(qglviewer::Vec(c.x, c.y, c.z));
    setSceneRadius(r);
    showEntireScene();
    
    glEnable(GL_DEPTH_TEST);
}

void DepthMapVis::drawMesh() {
    glEnable(GL_LIGHTING);
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_TRIANGLES);
    for (const auto& t : mesh.triangles) {
        const auto& v0 = mesh.vertices[t.v0]; const auto& v1 = mesh.vertices[t.v1]; const auto& v2 = mesh.vertices[t.v2];
        Vec3 n = computeFaceNormal(v0, v1, v2);
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(v0.x, v0.y, v0.z); glVertex3f(v1.x, v1.y, v1.z); glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();
}

void DepthMapVis::drawBoxPlanes() {
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // Xmin plane
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.min.z);
    glTexCoord2f(1,0); glVertex3f(box.min.x, box.min.y, box.max.z);
    glTexCoord2f(1,1); glVertex3f(box.min.x, box.max.y, box.max.z);
    glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.min.z);
    glEnd();
    
    // Xmax plane
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(box.max.x, box.min.y, box.min.z);
    glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.max.z);
    glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.max.z);
    glTexCoord2f(0,1); glVertex3f(box.max.x, box.max.y, box.min.z);
    glEnd();

    // Ymin plane
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.min.z);
    glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.min.z);
    glTexCoord2f(1,1); glVertex3f(box.max.x, box.min.y, box.max.z);
    glTexCoord2f(0,1); glVertex3f(box.min.x, box.min.y, box.max.z);
    glEnd();
    
    // Ymax plane
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(box.min.x, box.max.y, box.min.z);
    glTexCoord2f(1,0); glVertex3f(box.max.x, box.max.y, box.min.z);
    glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.max.z);
    glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.max.z);
    glEnd();

    // Zmin plane
    glBindTexture(GL_TEXTURE_2D, textures[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.min.z);
    glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.min.z);
    glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.min.z);
    glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.min.z);
    glEnd();
    
    // Zmax plane
    glBindTexture(GL_TEXTURE_2D, textures[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(box.min.x, box.min.y, box.max.z);
    glTexCoord2f(1,0); glVertex3f(box.max.x, box.min.y, box.max.z);
    glTexCoord2f(1,1); glVertex3f(box.max.x, box.max.y, box.max.z);
    glTexCoord2f(0,1); glVertex3f(box.min.x, box.max.y, box.max.z);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void DepthMapVis::draw() {
    drawMesh();
    
    if (showBox) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
        drawBoxPlanes();
    }
}

void DepthMapVis::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) exit(0);
    else if (e->key() == Qt::Key_B) {
        showBox = !showBox;
        update();
    }
    else QGLViewer::keyPressEvent(e);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model_file>" << std::endl;
        return 1;
    }
    QApplication app(argc, argv);
    DepthMapVis viewer(argv[1]);
    viewer.setWindowTitle("Bounding Box Depth Maps");
    viewer.resize(1000, 1000);
    viewer.show();
    return app.exec();
}
