cmake_minimum_required(VERSION 3.12)

project(RayTracing)

set(EXECUTABLE_NAME ${PROJECT_NAME})
set(OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(LIBRARY_DIRECTORY libraries)

set(SOURCE_FILES
    main.cpp
    scene.cpp
)
list(TRANSFORM SOURCE_FILES PREPEND src/)


if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /O2 /Ob2 /Oi /Ot /Oy /GT /GL /DNDEBUG /arch:AVX2 /fp:fast")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /O2 /Ob2 /Oi /Ot /Oy /GT /GL /DNDEBUG /arch:AVX2 /fp:fast")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mavx2")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -mavx2")
endif()


# Main executable

add_executable("${EXECUTABLE_NAME}" ${SOURCE_FILES})

set(CMAKE_CXX_EXTENSIONS OFF)
target_compile_features("${EXECUTABLE_NAME}" PUBLIC cxx_std_17)

set_target_properties("${EXECUTABLE_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
set_target_properties("${EXECUTABLE_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set_target_properties("${EXECUTABLE_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")


# GLM

add_subdirectory("${LIBRARY_DIRECTORY}/glm")
target_link_libraries("${EXECUTABLE_NAME}" glm)


# Linux TBB (for std::execution)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries("${EXECUTABLE_NAME}" tbb)
endif()
