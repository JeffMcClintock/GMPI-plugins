cmake_minimum_required(VERSION 3.15)

project(AGraphics)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP HAS_GUI BUILD_VST3_WRAPPER
     SOURCE_FILES 
        ${PROJECT_NAME}Gui.cpp
)
#
#
#
#
## helpful post-build-step: copy "$(OutDir)$(TargetName)$(TargetExt)" "C:\Program Files\Common Files\SynthEdit\modules\"
#
#set(sdk_srcs
#${gmpi_sdk_folder}/Core/Common.h
#${gmpi_sdk_folder}/Core/Common.cpp
#)
#set(srcs
#AGraphicsGui.cpp
#)
#
#include_directories(
#${sdk_folder}
#${gmpi_ui_folder}
#)
#
#source_group(se_sdk FILES ${sdk_srcs})
#
#include (GenerateExportHeader)
#
#add_library(${PROJECT_NAME} MODULE ${sdk_srcs} ${srcs})
#
#if(APPLE)
#  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
#  set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE TRUE)
#  set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "gmpi")
#
#  # Place xml file in bundle 'Resources' folder.
#  set(xml_path "${PROJECT_NAME}.xml")
#  target_sources(${PROJECT_NAME} PUBLIC ${xml_path})
#  set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
#endif()
#
#if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
#  set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#endif()
#
#set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".gmpi")
#
#add_custom_command(TARGET ${PROJECT_NAME}
#POST_BUILD
#COMMAND xcopy /c /y "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\SynthEdit\\modules\\"
#COMMENT "Copy to SEM folder"
#VERBATIM
#)
