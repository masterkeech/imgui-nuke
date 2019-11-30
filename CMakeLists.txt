cmake_minimum_required(VERSION 3.11)

project(nuke_plugins)

set(NUKE_VERSION 11.2v5)

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_SHARED_LIBRARY_PREFIX "")

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
endif()

add_subdirectory(third-party)

add_subdirectory(src)
