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

add_executable(ao_baker app/ambient_occlusion_baker.cpp src/MeshIO.cpp)
target_link_libraries(ao_baker PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sample_interior app/sample_interior.cpp src/MeshIO.cpp src/SampleInterior.cpp)
target_link_libraries(sample_interior PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sample_near_surface app/sample_near_surface.cpp src/MeshIO.cpp src/SampleSurface.cpp)
target_link_libraries(sample_near_surface PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sdf_gen app/sdf_generator.cpp src/MeshIO.cpp)
target_link_libraries(sdf_gen PRIVATE embree TBB::tbb assimp::assimp)

add_executable(caliper app/structural_caliper.cpp src/MeshIO.cpp src/SampleSurface.cpp src/StructuralCaliper.cpp)
target_link_libraries(caliper PRIVATE embree TBB::tbb assimp::assimp)

add_executable(curvature_analysis app/curvature_analysis.cpp src/MeshIO.cpp)
target_link_libraries(curvature_analysis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(symmetry_detection app/symmetry_detection.cpp src/MeshIO.cpp src/SampleSurface.cpp src/SymmetryDetector.cpp)
target_link_libraries(symmetry_detection PRIVATE embree TBB::tbb assimp::assimp)

add_executable(pocket_detection app/pocket_detection.cpp src/MeshIO.cpp)
target_link_libraries(pocket_detection PRIVATE embree TBB::tbb assimp::assimp)

add_executable(undercut_detection app/undercut_detection.cpp src/MeshIO.cpp)
target_link_libraries(undercut_detection PRIVATE embree TBB::tbb assimp::assimp)

add_executable(overhang_analysis app/overhang_analysis.cpp src/MeshIO.cpp)
target_link_libraries(overhang_analysis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(accessibility_analysis app/accessibility_analysis.cpp src/MeshIO.cpp)
target_link_libraries(accessibility_analysis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(assembly_clearance app/assembly_clearance.cpp src/MeshIO.cpp)
target_link_libraries(assembly_clearance PRIVATE embree TBB::tbb assimp::assimp)

add_executable(draft_angle_analysis app/draft_angle_analysis.cpp src/MeshIO.cpp)
target_link_libraries(draft_angle_analysis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(mesh_voxelizer app/mesh_voxelizer.cpp src/MeshIO.cpp)
target_link_libraries(mesh_voxelizer PRIVATE embree TBB::tbb assimp::assimp)

add_executable(projected_thickness app/projected_thickness.cpp src/MeshIO.cpp)
target_link_libraries(projected_thickness PRIVATE embree TBB::tbb assimp::assimp)

add_executable(mass_properties app/mass_properties.cpp src/MeshIO.cpp)
target_link_libraries(mass_properties PRIVATE embree TBB::tbb assimp::assimp)

add_executable(auto_orientation app/auto_orientation.cpp src/MeshIO.cpp)
target_link_libraries(auto_orientation PRIVATE embree TBB::tbb assimp::assimp)

add_executable(ray_traced_slicer app/ray_traced_slicer.cpp src/MeshIO.cpp)
target_link_libraries(ray_traced_slicer PRIVATE embree TBB::tbb assimp::assimp)

add_executable(cnc_toolpath_sim app/cnc_toolpath_sim.cpp src/MeshIO.cpp)
target_link_libraries(cnc_toolpath_sim PRIVATE embree TBB::tbb assimp::assimp)

add_executable(inter_part_visibility app/inter_part_visibility.cpp src/MeshIO.cpp)
target_link_libraries(inter_part_visibility PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sky_view_factor app/sky_view_factor.cpp src/MeshIO.cpp)
target_link_libraries(sky_view_factor PRIVATE embree TBB::tbb assimp::assimp)

add_executable(extraction_path_verification app/extraction_path_verification.cpp src/MeshIO.cpp)
target_link_libraries(extraction_path_verification PRIVATE embree TBB::tbb assimp::assimp)

add_executable(optimal_parting_line app/optimal_parting_line.cpp src/MeshIO.cpp)
target_link_libraries(optimal_parting_line PRIVATE embree TBB::tbb assimp::assimp)

add_executable(test_mass_sphere app/test_mass_sphere.cpp src/MeshIO.cpp)
target_link_libraries(test_mass_sphere PRIVATE embree TBB::tbb assimp::assimp)

add_executable(test_symmetry_sphere app/test_symmetry_sphere.cpp src/MeshIO.cpp src/SampleSurface.cpp src/SymmetryDetector.cpp)
target_link_libraries(test_symmetry_sphere PRIVATE embree TBB::tbb assimp::assimp)

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
