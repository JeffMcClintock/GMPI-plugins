cmake_minimum_required(VERSION 3.15)
project(CPP_Samples)

set(gmpi_sdk_folder
    ${CMAKE_BINARY_DIR}/_deps/gmpi-src/Core
    )

include_directories(
    ${gmpi_sdk_folder}
    )

add_subdirectory(Gain)
add_subdirectory(TestDatatypes)
# TODO add_subdirectory(AGraphics)
