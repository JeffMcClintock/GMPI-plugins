cmake_minimum_required(VERSION 3.15)

set(SE_LOCAL_BUILD FALSE CACHE BOOL "Execute extra build steps for developers machine")

include(FetchContent)

#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly include the GMPI examples)

FetchContent_Declare(
  gmpi
  GIT_REPOSITORY https://github.com/JeffMcClintock/GMPI
  GIT_TAG origin/main
  SOURCE_SUBDIR  Core
)

FetchContent_MakeAvailable(gmpi)

project(GMPI_Samples)

enable_testing()

set(CMAKE_CXX_STANDARD 17)

add_definitions(-D_UNICODE)
if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
endif()

if (MSVC)
    # Floating Point Model: Fast (/fp:fast)
    # Buffer Security Check: No (/GS-)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GS-")
endif()

include(${CMAKE_BINARY_DIR}/_deps/gmpi-src/gmpi_plugin.cmake)

add_subdirectory(plugins)
add_subdirectory(plugins_c)

