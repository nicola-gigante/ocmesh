cmake_minimum_required(VERSION 2.4)

set(name ocmesh)

set(SOURCE_FILES
        include/csg.h
        include/morton.h
        include/octree.h
        include/voxel.h

        src/octree.cpp
        src/obj.cpp
        src/csg.cpp
        src/csg_parser.cpp )

project(${name})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(include
        deps/cpputils/include
        deps/glm)

add_library(${name} ${SOURCE_FILES})

add_executable(tests test/main.cpp)
target_link_libraries(tests ${name})

