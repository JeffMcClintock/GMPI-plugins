cmake_minimum_required(VERSION 3.15)

project(AGraphics)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP HAS_GUI BUILD_VST3_WRAPPER
     SOURCE_FILES 
        ${PROJECT_NAME}.cpp
        ${PROJECT_NAME}Gui.cpp
)
