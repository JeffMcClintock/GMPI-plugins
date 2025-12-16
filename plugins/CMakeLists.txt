cmake_minimum_required(VERSION 3.30)
project(CPP_Samples)

include_directories(
    ${GMPI_SDK}/Core
    )

add_subdirectory(Gain)
add_subdirectory(GainGui)
add_subdirectory(SawDemo)
add_subdirectory(FreqAnalyser)
add_subdirectory(TestDatatypes)
