cmake_minimum_required(VERSION 3.16)
project(EmbreeTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Embree
find_package(Embree 4 REQUIRED)

# Find Qt6
find_package(Qt6 COMPONENTS Widgets Xml OpenGLWidgets REQUIRED)
find_package(OpenGL REQUIRED)

# Find Assimp
find_package(assimp REQUIRED)

include_directories(src)

# Existing targets
add_executable(meshvis app/mesh_visibility.cpp)
target_link_libraries(meshvis PRIVATE embree)

add_executable(sphere_benchmark app/sphere_benchmark.cpp)
target_link_libraries(sphere_benchmark PRIVATE embree)

add_executable(shape_diameter app/shape_diameter.cpp)
target_link_libraries(shape_diameter PRIVATE embree)

add_executable(raytracer app/raytracer.cpp)
target_link_libraries(raytracer PRIVATE embree)

add_executable(shadow_plane app/shadow_plane.cpp)
target_link_libraries(shadow_plane PRIVATE embree)

add_executable(depth_map app/depth_map.cpp)
target_link_libraries(depth_map PRIVATE embree)

add_executable(test_mesh_utils src/test_mesh_utils.cpp)

# New Visualization Target
add_executable(shape_diameter_vis app/ShapeDiameterVis.cpp)
target_include_directories(shape_diameter_vis PRIVATE /Users/csv610/Projects/Mesh/libQGLViewer)
target_link_libraries(shape_diameter_vis PRIVATE 
    embree 
    Qt6::Widgets 
    Qt6::Xml 
    Qt6::OpenGLWidgets 
    OpenGL::GL
    assimp::assimp
    /Users/csv610/Projects/Mesh/libQGLViewer/QGLViewer/libQGLViewer.dylib
)

enable_testing()
add_test(NAME MeshUtilsTest COMMAND test_mesh_utils)
