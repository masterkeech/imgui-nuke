include_directories(${IMGUI_INCLUDE_DIR})

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(NDKDIR /Applications/Nuke${NUKE_VERSION}/Nuke${NUKE_VERSION}.app/Contents/MacOS)
endif ()

link_directories(${NDKDIR})
link_directories(${NDKDIR}/../Frameworks)
include_directories(${NDKDIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../includes)

add_library(ImGuiDemo SHARED demo.cpp ${IMGUI_CPP_FILES})
if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(ImGuiDemo DDImage GLEW
            "-framework QuartzCore"
            "-framework IOKit"
            "-framework CoreFoundation"
            "-framework Carbon"
            "-framework ApplicationServices"
            "-framework OpenGL"
            "-framework AGL"
            )
elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(ImGuiDemo DDImage OpenGL)
endif ()

install(TARGETS ImGuiDemo DESTINATION ${CMAKE_INSTALL_PREFIX})
