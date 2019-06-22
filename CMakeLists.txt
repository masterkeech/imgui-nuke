cmake_minimum_required(VERSION 3.13)

project(nuke_plugins)

set(NUKE_VERSION 11.2v5)

set(CMAKE_CXX_STANDARD 11)

if(APPLE)
    SET(CMAKE_SHARED_LIBRARY_PREFIX "")
    SET(CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
endif()

add_subdirectory(third-party)

add_subdirectory(src)
