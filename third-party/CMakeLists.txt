# set the imgui version that you are using and add it to the third-party folder
set(IMGUI_VERSION 1.70)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui-${IMGUI_VERSION})
file(GLOB CPP_FILES ${IMGUI_DIR}/*.cpp PARENT_SCOPE)

set(IMGUI_INCLUDE_DIR ${IMGUI_DIR} PARENT_SCOPE)
set(IMGUI_CPP_FILES ${CPP_FILES} PARENT_SCOPE)
