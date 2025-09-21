cmake_minimum_required(VERSION 3.30)

project(Gain)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    FORMATS_LIST GMPI VST3
    SOURCE_FILES 
        ${PROJECT_NAME}.cpp
 )
