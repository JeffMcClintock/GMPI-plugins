cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum MacOS deployment version")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for MacOS" FORCE)
set(SE_LOCAL_BUILD FALSE CACHE BOOL "Execute extra build steps for developers machine")
set(GMPI_SDK_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI repo, or blank to fetch it automatically.")
set(GMPI_UI_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI UI repo, or blank to fetch it automatically.")
set(GMPI_WRAPPER_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI WRAPPERs repo, or blank to fetch it automatically.")
set(VST3_SDK_FOLDER_OVERRIDE "" CACHE PATH "path of a local vst3 repo, or blank to fetch it automatically.")

include(FetchContent)

# GMPI SDK
if("${GMPI_SDK_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching GMPI from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate projects for the GMPI examples)
FetchContent_Declare(
  gmpi
  GIT_REPOSITORY https://github.com/JeffMcClintock/GMPI
  GIT_TAG origin/main
  SOURCE_SUBDIR Core
)
FetchContent_MakeAvailable(gmpi)
set(gmpi_sdk_folder ${gmpi_SOURCE_DIR})
else()
message(STATUS "Using local GMPI folder")
set(gmpi_sdk_folder ${GMPI_SDK_FOLDER_OVERRIDE})
endif()

# GMPI-UI SDK
if("${GMPI_UI_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching GMPI-UI from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate projects for the GMPI examples)
FetchContent_Declare(
  gmpi_ui
  GIT_REPOSITORY https://github.com/JeffMcClintock/gmpi_ui
  GIT_TAG origin/main
  SOURCE_SUBDIR helpers
)
FetchContent_MakeAvailable(gmpi_ui)
set(gmpi_ui_folder ${gmpi_ui_SOURCE_DIR})
else()
message(STATUS "Using local GMPI-UI folder")
set(gmpi_ui_folder ${GMPI_UI_FOLDER_OVERRIDE})
endif()

# GMPI WRAPPERS
if("${GMPI_WRAPPER_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching GMPI WRAPPERS from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate projects for the GMPI examples)
FetchContent_Declare(
  gmpi_wrappers
  GIT_REPOSITORY https://github.com/JeffMcClintock/GMPI_Wrappers
  GIT_TAG origin/main
  SOURCE_SUBDIR Core
)
FetchContent_MakeAvailable(gmpi_wrappers)
set(GMPI_ADAPTORS ${gmpi_wrappers_SOURCE_DIR})
else()
message(STATUS "Using local GMPI WRAPPERS folder")
set(GMPI_ADAPTORS ${GMPI_WRAPPER_FOLDER_OVERRIDE})
endif()

# GET VST3 SDK
if("${VST3_SDK_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching VST3 from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate projects for the GMPI examples)
FetchContent_Declare(
  vst3
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
  GIT_TAG origin/master
  SOURCE_SUBDIR tutorials
)
FetchContent_MakeAvailable(vst3)
set(VST3_SDK ${vst3_SOURCE_DIR})
else()
message(STATUS "Using local VST3 folder")
set(VST3_SDK ${VST3_SDK_FOLDER_OVERRIDE})
endif()

# plugins that want to link to the VST3 SDK need to be able to include files from the VST3 SDK
include_directories(
${VST3_SDK}
)

##################################################################################

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

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()

include(${gmpi_sdk_folder}/gmpi_plugin.cmake)

add_subdirectory(plugins)
add_subdirectory(plugins_c)
add_subdirectory(${GMPI_ADAPTORS} GMPI_Wrappers)

