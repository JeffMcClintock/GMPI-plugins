cmake_minimum_required(VERSION 3.15)

project(Gain)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP BUILD_VST3_WRAPPER
    SOURCE_FILES 
        ${PROJECT_NAME}.cpp
 )

 #LINK TEH VST3 wrapper as a static library (I changed the wrapper CMakeLists.txt to "add_library(${PROJECT_NAME} STATIC"" to make this work)
 target_link_libraries(${PROJECT_NAME} PUBLIC SynthEdit_VST3)
 # TODO gmpi_vst3_adaptor)