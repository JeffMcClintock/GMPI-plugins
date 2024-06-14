cmake_minimum_required(VERSION 3.15)
project(CPP_Samples)

include_directories(
    ${gmpi_sdk_folder}/Core
    )

add_subdirectory(Gain)
add_subdirectory(TestDatatypes)
add_subdirectory(AGraphics)
