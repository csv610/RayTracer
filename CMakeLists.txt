cmake_minimum_required(VERSION 3.16)
project(EmbreeTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Dependencies
find_package(Embree 4 REQUIRED)
find_package(Qt6 COMPONENTS Widgets Xml OpenGLWidgets REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(TBB REQUIRED)

include_directories(src)

# Existing targets
add_executable(meshvis app/mesh_visibility.cpp src/MeshIO.cpp)
target_link_libraries(meshvis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sphere_benchmark app/sphere_benchmark.cpp src/MeshIO.cpp)
target_link_libraries(sphere_benchmark PRIVATE embree TBB::tbb assimp::assimp)

add_executable(shape_diameter app/shape_diameter.cpp src/MeshIO.cpp)
target_link_libraries(shape_diameter PRIVATE embree TBB::tbb assimp::assimp)

add_executable(raytracer app/raytracer.cpp src/MeshIO.cpp)
target_link_libraries(raytracer PRIVATE embree TBB::tbb assimp::assimp)

add_executable(shadow_plane app/shadow_plane.cpp src/MeshIO.cpp)
target_link_libraries(shadow_plane PRIVATE embree TBB::tbb assimp::assimp)

add_executable(depth_map app/depth_map.cpp src/MeshIO.cpp)
target_link_libraries(depth_map PRIVATE embree TBB::tbb assimp::assimp)

add_executable(test_mesh_utils src/test_mesh_utils.cpp src/MeshIO.cpp)
target_link_libraries(test_mesh_utils PRIVATE assimp::assimp)

# New Visualization Target
add_executable(shape_diameter_vis app/ShapeDiameterVis.cpp src/MeshIO.cpp)
target_include_directories(shape_diameter_vis PRIVATE /Users/csv610/Projects/Mesh/libQGLViewer)
target_link_libraries(shape_diameter_vis PRIVATE 
    embree 
    Qt6::Widgets 
    Qt6::Xml 
    Qt6::OpenGLWidgets 
    OpenGL::GL
    assimp::assimp
    TBB::tbb
    /Users/csv610/Projects/Mesh/libQGLViewer/QGLViewer/libQGLViewer.dylib
)

# Depth Map Viewer Target
add_executable(depth_map_viewer app/depth_map_viewer.cpp src/MeshIO.cpp)
target_include_directories(depth_map_viewer PRIVATE /Users/csv610/Projects/Mesh/libQGLViewer)
target_link_libraries(depth_map_viewer PRIVATE 
    embree 
    assimp::assimp
    Qt6::Widgets 
    Qt6::Xml 
    Qt6::OpenGLWidgets 
    OpenGL::GL
    TBB::tbb
    /Users/csv610/Projects/Mesh/libQGLViewer/QGLViewer/libQGLViewer.dylib
)

enable_testing()
add_test(NAME MeshUtilsTest COMMAND test_mesh_utils)
