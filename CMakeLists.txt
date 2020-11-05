cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0043 NEW)
project(BluenoiseMaskOptimizer VERSION 1.0.0 DESCRIPTION "")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(-DPROJECT_ROOT="${CMAKE_SOURCE_DIR}/")

add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/glfw)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/glfw/include)

file(GLOB sources src/glad/glad.c src/*.cpp)

set(exec Optimizer)
add_executable(${exec} ${sources})
target_link_libraries(${exec} glfw)

add_dependencies(${exec} glfw)

find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
    message(FATAL_ERROR "OpenMP is required for the benchmark to build")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        set(flags "/W3" "/openmp")
    else()
        set(flags "-g" "-Wall" "-std=c++14" "-fopenmp")
    endif()
else()
    if(MSVC)
        set(flags "/O2" "/W3" "/openmp")
    else()
        set(flags "-O3" "-Wall" "-fopenmp")
    endif()
endif()

target_compile_features(${exec} PRIVATE cxx_std_14) 
target_compile_options(${exec} PRIVATE ${flags})

