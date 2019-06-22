include_directories(${IMGUI_INCLUDE_DIR})

if (APPLE)
    set(NDKDIR /Applications/Nuke${NUKE_VERSION}/Nuke${NUKE_VERSION}.app/Contents/MacOS)
endif ()

link_directories(${NDKDIR})
link_directories(${NDKDIR}/../Frameworks)
include_directories(${NDKDIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../includes)

add_library(ImGuiDemo SHARED demo.cpp ${IMGUI_CPP_FILES})
target_link_libraries(ImGuiDemo DDImage GLEW
        "-framework QuartzCore"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework Carbon"
        "-framework ApplicationServices"
        "-framework OpenGL"
        "-framework AGL"
        )
