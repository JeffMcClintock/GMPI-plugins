cmake_minimum_required(VERSION 3.15)

project(GainGui)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP HAS_GUI BUILD_VST3_WRAPPER
     SOURCE_FILES 
        Gain.cpp
        GainGui.cpp
)
