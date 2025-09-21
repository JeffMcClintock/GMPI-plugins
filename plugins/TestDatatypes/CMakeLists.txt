cmake_minimum_required(VERSION 3.30)

project(TestDatatypes)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES 
        ${PROJECT_NAME}.cpp
 )
