# imgui-nuke
A simple header only implementation of imgui for Foundry's Nuke software

# installation

## imgui

Download the desired version of imgui from https://github.com/ocornut/imgui and install the headers into the third-party directory. Set the IMGUI_VERSION CMake variable to point at the installed version number, eg. 1.70.

## nuke
### mac osx
Set the NUKE_VERSION CMake variable to the version of Nuke to compile against, eg. 11.2v5.
### linux
Cmake has not yet been setup for compiling and running against Nuke.

# usage
See demo.cpp as a simple example of how to compile against the imgui_nuke header and add imgui to your plugins.
