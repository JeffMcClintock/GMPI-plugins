cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum MacOS deployment version")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for MacOS" FORCE)
set(SE_LOCAL_BUILD FALSE CACHE BOOL "Execute extra build steps for developers machine")
set(GMPI_SDK_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI repo, or blank to fetch it automatically.")
set(GMPI_UI_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI UI repo, or blank to fetch it automatically.")
set(GMPI_WRAPPER_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI WRAPPERs repo, or blank to fetch it automatically.")
set(AU2_SDK_FOLDER_OVERRIDE "" CACHE PATH "path of a local Audio Unit SDK repo, or blank to fetch it automatically.")

include(FetchContent)

# CPM bootstrap (shared cache across builds)
set(CPM_DOWNLOAD_VERSION 0.38.6)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
set(CPM_GIT_SHALLOW ON) # shallow clone for speed

if(NOT DEFINED CPM_SOURCE_CACHE)
  if(WIN32)
    set(CPM_SOURCE_CACHE "$ENV{USERPROFILE}/.cpm" CACHE STRING "CPM cache dir")
  else()
    set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/CPM" CACHE STRING "CPM cache dir")
  endif()
endif()

set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
    "${CPM_DOWNLOAD_LOCATION}"
    TLS_VERIFY ON
  )
endif()
include("${CPM_DOWNLOAD_LOCATION}")

# Fetch VST3 SDK via CPM
CPMAddPackage(
NAME vst3_sdk
GITHUB_REPOSITORY steinbergmedia/vst3sdk
GIT_TAG v3.7.14_build_55       # or a specific commit SHA
DOWNLOAD_ONLY YES              # we only need sources; actual add_subdirectory happens later
)
set(VST3_SDK ${vst3_sdk_SOURCE_DIR})

# Fetch CLAP SDK via CPM
CPMAddPackage(
NAME clap_sdk
GITHUB_REPOSITORY free-audio/clap
GIT_TAG 1.2.6                  # or a specific commit SHA
DOWNLOAD_ONLY YES              # we only need sources; actual add_subdirectory happens later
)
set(CLAP_SDK ${clap_sdk_SOURCE_DIR})

# Fetch CLAP helpers via CPM
CPMAddPackage(
NAME clap_helpers
GITHUB_REPOSITORY free-audio/clap-helpers
GIT_TAG 58ab81b                 # or a specific commit SHA
DOWNLOAD_ONLY YES              # we only need sources; actual add_subdirectory happens later
)
set(CLAP_HELPERS_SDK ${clap_helpers_SOURCE_DIR})


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

# GET AU2 SDK
if(APPLE)
if("${AU2_SDK_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching AudioUnit SDK from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate example projects)
FetchContent_Declare(
  au2
  GIT_REPOSITORY https://github.com/apple/AudioUnitSDK
  GIT_TAG origin/main
  SOURCE_SUBDIR hooks
)
FetchContent_MakeAvailable(au2)
set(AU_SDK ${au2_SOURCE_DIR})
set(AU_SDK_SRC ${au2_SOURCE_DIR}/src)
set(AU_SDK_H ${au2_SOURCE_DIR}/include)
else()
message(STATUS "Using local AudioUnit folder")
set(AU_SDK ${AU2_SDK_FOLDER_OVERRIDE})
endif()
endif() # APPLE

##################################################################################

project(GMPI_Samples)

enable_testing()

set(CMAKE_CXX_STANDARD 20)

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

