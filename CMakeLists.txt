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

include_directories(src include)

# Core Analysis Classes
set(ANALYSIS_SOURCES 
    src/MeshIO.cpp 
    src/RayTracer.cpp
    src/Renderer.cpp
    src/SampleSurface.cpp 
    src/SampleInterior.cpp
    src/StructuralCaliper.cpp 
    src/SymmetryDetector.cpp
    src/ManufacturingAnalyzer.cpp
    src/AssemblyAnalyzer.cpp
    src/GeometryAnalyzer.cpp
    src/PhysicalProperties.cpp
    src/SimulationSuite.cpp
    src/AutoOrientationOptimizer.cpp
    src/AccessibilityAnalysis.cpp
    src/ThicknessAnalyzer.cpp
    src/CurvatureAnalyzer.cpp
    src/VisibilityAnalyzer.cpp
    src/RayTracedSlicer.cpp
    src/InsideOutsideKernel.cpp
)

# Applications (Thin wrappers around classes)
add_executable(raytracer app/raytracer_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(raytracer PRIVATE embree TBB::tbb assimp::assimp)

add_executable(ao_baker app/ambient_occlusion_baker_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(ao_baker PRIVATE embree TBB::tbb assimp::assimp)

add_executable(caliper app/structural_caliper_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(caliper PRIVATE embree TBB::tbb assimp::assimp)

add_executable(symmetry_detection app/symmetry_detection_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(symmetry_detection PRIVATE embree TBB::tbb assimp::assimp)

add_executable(undercut_detection app/undercut_detection_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(undercut_detection PRIVATE embree TBB::tbb assimp::assimp)

add_executable(overhang_analysis app/overhang_analysis_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(overhang_analysis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(draft_angle_analysis app/draft_angle_analysis_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(draft_angle_analysis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(optimal_parting_line app/optimal_parting_line_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(optimal_parting_line PRIVATE embree TBB::tbb assimp::assimp)

add_executable(mass_properties app/mass_properties_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(mass_properties PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sky_view_factor app/sky_view_factor_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(sky_view_factor PRIVATE embree TBB::tbb assimp::assimp)

add_executable(pocket_detection app/pocket_detection_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(pocket_detection PRIVATE embree TBB::tbb assimp::assimp)

add_executable(accessibility_analysis app/accessibility_analysis_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(accessibility_analysis PRIVATE embree TBB::tbb assimp::assimp)

add_executable(assembly_clearance app/assembly_clearance_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(assembly_clearance PRIVATE embree TBB::tbb assimp::assimp)

add_executable(extraction_path_verification app/extraction_path_verification_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(extraction_path_verification PRIVATE embree TBB::tbb assimp::assimp)

add_executable(auto_orientation app/auto_orientation_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(auto_orientation PRIVATE embree TBB::tbb assimp::assimp)

add_executable(ray_traced_slicer app/ray_traced_slicer_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(ray_traced_slicer PRIVATE embree TBB::tbb assimp::assimp)

add_executable(cnc_toolpath_sim app/cnc_toolpath_sim_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(cnc_toolpath_sim PRIVATE embree TBB::tbb assimp::assimp)

add_executable(inter_part_visibility app/inter_part_visibility_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(inter_part_visibility PRIVATE embree TBB::tbb assimp::assimp)

add_executable(mesh_voxelizer app/mesh_voxelizer_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(mesh_voxelizer PRIVATE embree TBB::tbb assimp::assimp)

add_executable(mesh_visibility app/mesh_visibility_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(mesh_visibility PRIVATE embree TBB::tbb assimp::assimp)

add_executable(projected_thickness app/projected_thickness_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(projected_thickness PRIVATE embree TBB::tbb assimp::assimp)

add_executable(curvature_analysis app/curvature_analysis_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(curvature_analysis PRIVATE embree TBB::tbb assimp::assimp)

# original apps
add_executable(sphere_benchmark app/sphere_benchmark_cli.cpp src/MeshIO.cpp src/RayTracer.cpp)
target_link_libraries(sphere_benchmark PRIVATE embree TBB::tbb assimp::assimp)
add_executable(shape_diameter app/shape_diameter_cli.cpp src/MeshIO.cpp src/RayTracer.cpp src/ShapeDiameter.cpp)
target_link_libraries(shape_diameter PRIVATE embree TBB::tbb assimp::assimp)
add_executable(shadow_plane app/shadow_plane_cli.cpp src/MeshIO.cpp src/RayTracer.cpp)
target_link_libraries(shadow_plane PRIVATE embree TBB::tbb assimp::assimp)
add_executable(depth_map app/depth_map_cli.cpp src/MeshIO.cpp src/RayTracer.cpp)
target_link_libraries(depth_map PRIVATE embree TBB::tbb assimp::assimp)
add_executable(sdf_gen app/sdf_generator_cli.cpp src/MeshIO.cpp src/RayTracer.cpp)
target_link_libraries(sdf_gen PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sample_interior app/sample_interior_cli.cpp src/SampleInterior.cpp src/MeshIO.cpp src/RayTracer.cpp)
target_link_libraries(sample_interior PRIVATE embree TBB::tbb assimp::assimp)

add_executable(sample_near_surface app/sample_near_surface_cli.cpp src/SampleSurface.cpp src/MeshIO.cpp src/RayTracer.cpp)
target_link_libraries(sample_near_surface PRIVATE embree TBB::tbb assimp::assimp)

# Unit Tests
add_executable(gen_test_data tests/gen_test_data.cpp src/MeshIO.cpp)
target_link_libraries(gen_test_data PRIVATE assimp::assimp)

add_executable(test_mesh_utils src/test_mesh_utils.cpp src/MeshIO.cpp)
target_link_libraries(test_mesh_utils PRIVATE assimp::assimp)
add_executable(test_mass_sphere tests/test_mass_sphere_cli.cpp src/MeshIO.cpp)
target_link_libraries(test_mass_sphere PRIVATE embree TBB::tbb assimp::assimp)
add_executable(test_symmetry_sphere tests/test_symmetry_sphere_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(test_symmetry_sphere PRIVATE embree TBB::tbb assimp::assimp)

add_executable(test_apps tests/test_apps.cpp ${ANALYSIS_SOURCES})
target_link_libraries(test_apps PRIVATE embree TBB::tbb assimp::assimp)

add_executable(test_kernel_cli app/test_kernel_cli.cpp ${ANALYSIS_SOURCES})
target_link_libraries(test_kernel_cli PRIVATE embree TBB::tbb assimp::assimp)

# Visualization
add_executable(shape_diameter_vis app/shape_diameter_vis_cli.cpp src/MeshIO.cpp src/RayTracer.cpp src/ShapeDiameter.cpp)
target_include_directories(shape_diameter_vis PRIVATE /Users/csv610/Projects/Mesh/libQGLViewer)
target_link_libraries(shape_diameter_vis PRIVATE embree Qt6::Widgets Qt6::Xml Qt6::OpenGLWidgets OpenGL::GL assimp::assimp TBB::tbb /Users/csv610/Projects/Mesh/libQGLViewer/QGLViewer/libQGLViewer.dylib)

add_executable(depth_map_viewer app/depth_map_viewer_cli.cpp src/MeshIO.cpp src/RayTracer.cpp src/ShapeDiameter.cpp)
target_include_directories(depth_map_viewer PRIVATE /Users/csv610/Projects/Mesh/libQGLViewer)
target_link_libraries(depth_map_viewer PRIVATE embree assimp::assimp Qt6::Widgets Qt6::Xml Qt6::OpenGLWidgets OpenGL::GL TBB::tbb /Users/csv610/Projects/Mesh/libQGLViewer/QGLViewer/libQGLViewer.dylib)

add_executable(mesh_slicer_viewer app/mesh_slicer_viewer.cpp ${ANALYSIS_SOURCES})
target_include_directories(mesh_slicer_viewer PRIVATE /Users/csv610/Projects/Mesh/libQGLViewer)
target_link_libraries(mesh_slicer_viewer PRIVATE embree Qt6::Widgets Qt6::Xml Qt6::OpenGLWidgets OpenGL::GL assimp::assimp TBB::tbb /Users/csv610/Projects/Mesh/libQGLViewer/QGLViewer/libQGLViewer.dylib)

enable_testing()

# Generate test data first
add_test(NAME GenTestData COMMAND gen_test_data)
set_tests_properties(GenTestData PROPERTIES FIXTURES_SETUP TestData)

add_test(NAME MeshUtilsTest COMMAND test_mesh_utils)
add_test(NAME AppsTest COMMAND test_apps)
add_test(NAME MassSphereTest COMMAND test_mass_sphere)
add_test(NAME SymmetrySphereTest COMMAND test_symmetry_sphere)
add_test(NAME KernelTest COMMAND test_kernel_cli)

# Functional tests (python scripts)
file(GLOB TEST_SCRIPTS tests/*.py)
foreach(script ${TEST_SCRIPTS})
    get_filename_component(script_name ${script} NAME_WE)
    add_test(NAME ${script_name} COMMAND python3 ${script})
    set_tests_properties(${script_name} PROPERTIES FIXTURES_REQUIRED TestData)
endforeach()
