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

class ShapeDiameter {
public:
    ShapeDiameter(const std::vector<Vertex>& vertices, const std::vector<Triangle>& triangles)
        : vertices(vertices), triangles(triangles) {
        device = rtcNewDevice(nullptr);
        if (device == nullptr) {
            throw std::runtime_error("Failed to create Embree device");
        }
        rtcSetDeviceErrorFunction(device, [](void* userPtr, RTCError code, const char* msg) {
            printf("Embree error %d: %s\n", code, msg);
        }, nullptr);
        scene = rtcNewScene(device);
        buildScene();
    }

    ~ShapeDiameter() {
        rtcReleaseScene(scene);
        rtcReleaseDevice(device);
    }

    struct RayHit {
        int primID;
        Vec3 dir;
        float distance;
    };

    void computeForFace(int triIdx, std::vector<RayHit>& hits, int numTheta = 4, int numPhi = 8, float coneAngle = 0.5f) {
        hits.clear();
        if (triIdx < 0 || triIdx >= (int)triangles.size()) return;

        const Triangle& tri = triangles[triIdx];
        Vec3 normal = computeFaceNormal(vertices[tri.v0], vertices[tri.v1], vertices[tri.v2]);
        Vec3 inwardNormal = {-normal.x, -normal.y, -normal.z};
        Vec3 faceCenter = computeFaceCenter(vertices[tri.v0], vertices[tri.v1], vertices[tri.v2]);

        Vec3 up = (std::abs(inwardNormal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {inwardNormal.y * up.z - inwardNormal.z * up.y,
                        inwardNormal.z * up.x - inwardNormal.x * up.z,
                        inwardNormal.x * up.y - inwardNormal.y * up.x};
        float tLen = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
        tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen;
        Vec3 bitangent = {inwardNormal.y * tangent.z - inwardNormal.z * tangent.y,
                          inwardNormal.z * tangent.x - inwardNormal.x * tangent.z,
                          inwardNormal.x * tangent.y - inwardNormal.y * tangent.x};

        for (int i = 0; i < numTheta; ++i) {
            for (int j = 0; j < numPhi; ++j) {
                float theta = coneAngle * (i + 0.5f) / numTheta; 
                float phi = 2.0f * 3.14159f * (j + 0.5f) / numPhi;
                float sinT = sin(theta); float cosT = cos(theta);
                float sinP = sin(phi);   float cosP = cos(phi);

                Vec3 rayDir = {
                    (tangent.x * cosP + bitangent.x * sinP) * sinT + inwardNormal.x * cosT,
                    (tangent.y * cosP + bitangent.y * sinP) * sinT + inwardNormal.y * cosT,
                    (tangent.z * cosP + bitangent.z * sinP) * sinT + inwardNormal.z * cosT
                };

                RTCRayHit rh;
                float epsilon = 0.0001f;
                rh.ray.org_x = faceCenter.x + inwardNormal.x * epsilon;
                rh.ray.org_y = faceCenter.y + inwardNormal.y * epsilon;
                rh.ray.org_z = faceCenter.z + inwardNormal.z * epsilon;
                rh.ray.dir_x = rayDir.x; rh.ray.dir_y = rayDir.y; rh.ray.dir_z = rayDir.z;
                rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = 0xFFFFFFFF;
                rh.ray.time = 0.0f; rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

                RTCIntersectArguments args;
                rtcInitIntersectArguments(&args);
                rtcIntersect1(scene, &rh, &args);

                if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                    hits.push_back({(int)rh.hit.primID, rayDir, rh.ray.tfar});
                }
            }
        }
    }

private:
    const std::vector<Vertex>& vertices;
    const std::vector<Triangle>& triangles;
    RTCDevice device;
    RTCScene scene;

    void buildScene() {
        RTCGeometry triangleMesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

        Vertex* vertBuffer = (Vertex*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i) vertBuffer[i] = vertices[i];

        Triangle* triBuffer = (Triangle*)rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), triangles.size());
        for (size_t i = 0; i < triangles.size(); ++i) triBuffer[i] = triangles[i];

        rtcSetGeometryBuildQuality(triangleMesh, RTC_BUILD_QUALITY_HIGH);
        rtcCommitGeometry(triangleMesh);
        rtcAttachGeometry(scene, triangleMesh);
        rtcReleaseGeometry(triangleMesh);
        rtcCommitScene(scene);
    }
};

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
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    
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

    vertices.clear();
    triangles.clear();

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        unsigned int offset = vertices.size();

        for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            vertices.push_back({mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z});
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
            aiFace face = mesh->mFaces[j];
            if (face.mNumIndices == 3) {
                triangles.push_back({face.mIndices[0] + offset, face.mIndices[1] + offset, face.mIndices[2] + offset});
            }
        }
    }
    std::cout << "Mesh loaded with Assimp: " << vertices.size() << " vertices, " << triangles.size() << " triangles" << std::endl;
}

void ShapeDiameterVis::init() {
    loadMesh();
    sd = new ShapeDiameter(vertices, triangles);
    
    // Set scene center and radius for rotation and camera fit
    if (!vertices.empty()) {
        Vec3 minP = {vertices[0].x, vertices[0].y, vertices[0].z};
        Vec3 maxP = {vertices[0].x, vertices[0].y, vertices[0].z};
        for (const auto& v : vertices) {
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
    for (size_t i = 0; i < triangles.size(); ++i) {
        bool isHit = std::find(hitTriangles.begin(), hitTriangles.end(), (int)i) != hitTriangles.end();
        if ((int)i != selectedTriangle && !isHit) {
            glColor3f(0.8f, 0.8f, 0.8f); // Default: Light Gray
            const Triangle& t = triangles[i];
            const Vertex& v0 = vertices[t.v0];
            const Vertex& v1 = vertices[t.v1];
            const Vertex& v2 = vertices[t.v2];
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
    for (size_t i = 0; i < triangles.size(); ++i) {
        bool isHit = std::find(hitTriangles.begin(), hitTriangles.end(), (int)i) != hitTriangles.end();
        if (isHit && (int)i != selectedTriangle) {
            glColor3f(1.0f, 0.0f, 0.0f);
            const Triangle& t = triangles[i];
            const Vertex& v0 = vertices[t.v0];
            const Vertex& v1 = vertices[t.v1];
            const Vertex& v2 = vertices[t.v2];
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
        const Triangle& t = triangles[selectedTriangle];
        const Vertex& v0 = vertices[t.v0];
        const Vertex& v1 = vertices[t.v1];
        const Vertex& v2 = vertices[t.v2];
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
    Vec3 faceCenter = computeFaceCenter(vertices[triangles[selectedTriangle].v0], 
                                         vertices[triangles[selectedTriangle].v1], 
                                         vertices[triangles[selectedTriangle].v2]);
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
        for (size_t i = 0; i < triangles.size(); ++i) {
            Vec3 center = computeFaceCenter(vertices[triangles[i].v0], vertices[triangles[i].v1], vertices[triangles[i].v2]);
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
