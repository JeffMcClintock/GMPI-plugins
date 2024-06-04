cmake_minimum_required(VERSION 3.15)

project(Gain)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP BUILD_VST3_WRAPPER
    SOURCE_FILES 
        ${PROJECT_NAME}.cpp
 )
