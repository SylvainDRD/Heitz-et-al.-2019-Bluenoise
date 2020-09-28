cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0043 NEW)
project(BluenoiseMaskOptimizer VERSION 1.0.0 DESCRIPTION "GPU based implementation of an N dimensional bluenoise mask generator")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(-DPROJECT_ROOT="${CMAKE_SOURCE_DIR}/")

add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/glfw)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/glfw/include)

file(GLOB sources src/glad/glad.c src/*.cpp)

set(exec BluenoiseMaskOptimizer)
add_executable(${exec} ${sources})
target_link_libraries(${exec} glfw)

add_dependencies(${exec} glfw)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    if(MSVC)
        set(flags "/W4")
    else()
        set(flags "-g" "-Wall")
    endif()
else()
    if(MSVC)
        set(flags "/O2" "/W4")
    else()
        set(flags "-O3" "-Wall")
    endif()
endif()

target_compile_features(${exec} PRIVATE cxx_std_14) 
target_compile_options(${exec} PRIVATE ${flags})

