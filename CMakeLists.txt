cmake_minimum_required(VERSION 3.16)
project(EmbreeTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Embree 4 REQUIRED)

include_directories(src)

add_executable(meshvis app/mesh_visibility.cpp)
target_link_libraries(meshvis PRIVATE embree)

# test_embree removed - source file missing

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

enable_testing()
add_test(NAME MeshUtilsTest COMMAND test_mesh_utils)
