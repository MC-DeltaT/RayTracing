cmake_minimum_required(VERSION 3.12)

project(RayTracing)

set(EXECUTABLE_NAME ${PROJECT_NAME})
set(OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(LIBRARY_DIRECTORY libraries)

set(SOURCE_FILES
    camera.cpp
    image.cpp
    main.cpp
    mesh.cpp
    ray.cpp
    render.cpp
    scene.cpp
)
list(TRANSFORM SOURCE_FILES PREPEND src/)


# Main executable

add_executable("${EXECUTABLE_NAME}" ${SOURCE_FILES})

target_link_libraries("${EXECUTABLE_NAME}" glm boost)

set(CMAKE_CXX_EXTENSIONS OFF)
target_compile_features("${EXECUTABLE_NAME}" PUBLIC cxx_std_17)

set_target_properties("${EXECUTABLE_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set_target_properties("${EXECUTABLE_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set_target_properties("${EXECUTABLE_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")


# GLM

add_subdirectory("${LIBRARY_DIRECTORY}/glm")


# Boost

add_subdirectory("${LIBRARY_DIRECTORY}/boost")
