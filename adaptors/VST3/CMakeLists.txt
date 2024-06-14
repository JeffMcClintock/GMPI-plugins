cmake_minimum_required(VERSION 3.15)

set(VST3_SDK_FOLDER_OVERRIDE "" CACHE PATH "path of a local vst3 repo, or blank to fetch it automatically.")
set(SE_SDK_FOLDER_OVERRIDE "" CACHE PATH "path of a local SynthEdit SDK repo, or blank to fetch it automatically.")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

include(FetchContent)

# GET VST3 SDK
if("${VST3_SDK_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching VST3 from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate projects for the GMPI examples)
FetchContent_Declare(
  vst3
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
  GIT_TAG origin/master
  SOURCE_SUBDIR vst3_doc
)
FetchContent_MakeAvailable(vst3)
set(VST3_SDK ${vst3_SOURCE_DIR})
else()
message(STATUS "Using local VST3 folder")
set(VST3_SDK ${VST3_SDK_FOLDER_OVERRIDE})
endif()

# GET SYNTHEDIT SDK
if("${SE_SDK_FOLDER_OVERRIDE}" STREQUAL "")
FetchContent_Declare(
  syntheditsdk
  GIT_REPOSITORY https://github.com/JeffMcClintock/SynthEdit_SDK
  GIT_TAG origin/master
  SOURCE_SUBDIR se_sdk3
)

FetchContent_MakeAvailable(syntheditsdk)

set(se_sdk_folder
	${syntheditsdk_SOURCE_DIR}/se_sdk3
    )
set(se_shared_folder
    ${syntheditsdk_SOURCE_DIR}/shared
    )
set(se_sdk_hosting_folder
    ${syntheditsdk_SOURCE_DIR}/se_sdk3_hosting
    )
else()
message(STATUS "Using local SynthEdit modules folder")
set(se_sdk_folder ${SE_SDK_FOLDER_OVERRIDE}/se_sdk3)
set(se_shared_folder ${SE_SDK_FOLDER_OVERRIDE}/shared)
set(se_sdk_hosting_folder ${SE_SDK_FOLDER_OVERRIDE}/se_sdk3_hosting)
# hosting classes rewritten for GMPI not SE_SDK3 anymore
endif()

project(SynthEdit_VST3)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-D_UNICODE)
add_definitions(-DSE_TARGET_PLUGIN)
add_definitions(-DSE_TARGET_VST3)
add_definitions(-DGMPI_VST3_WRAPPER)

if (MSVC)
    # Floating Point Model: Fast (/fp:fast)
    # Buffer Security Check: No (/GS-)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GS-")
endif()

#set(DSP_CORE ${CMAKE_CURRENT_SOURCE_DIR}/../SE_DSP_CORE)
#set(VST3_SDK ${CMAKE_CURRENT_SOURCE_DIR}/../SDKs/VST3_SDK)


#${se_sdk_folder}/mp_sdk_common.h
#${se_sdk_folder}/mp_sdk_common.cpp

set(srcs
factory.cpp
MyVstPluginFactory.h
MyVstPluginFactory.cpp
StagingMemoryBuffer.h
StagingMemoryBuffer.h
adelayprocessor.h
adelayprocessor.cpp
adelaycontroller.h
adelaycontroller.cpp
MyTypeTraits.h
MyTypeTraits.cpp
interThreadQue.h
interThreadQue.cpp
${se_shared_folder}/FileFinder.h
${se_shared_folder}/FileFinder.cpp
${se_shared_folder}/FileWatcher.h
${se_shared_folder}/FileWatcher.cpp
${se_shared_folder}/xp_dynamic_linking.h
${se_shared_folder}/xp_dynamic_linking.cpp
conversion.h
conversion.cpp
HostControls.h
HostControls.cpp
RawConversions.h
RawConversions.cpp
my_input_stream.h
my_input_stream.cpp
interThreadQue.cpp
interThreadQue.h
tinyxml2/tinyxml2.h
tinyxml2/tinyxml2.cpp
${se_sdk_folder}/it_enum_list.h
${se_sdk_folder}/it_enum_list.cpp
${se_sdk_hosting_folder}/BundleInfo.h
${se_sdk_hosting_folder}/BundleInfo.cpp
${se_sdk_hosting_folder}/ProcessorStateManager.h
${se_sdk_hosting_folder}/ProcessorStateManager.cpp
${ADAPTORS_COMMON_FOLDER}/MpParameter.h
${ADAPTORS_COMMON_FOLDER}/MpParameter.cpp
${ADAPTORS_COMMON_FOLDER}/GmpiResourceManager.h
${ADAPTORS_COMMON_FOLDER}/GmpiResourceManager.cpp
${ADAPTORS_COMMON_FOLDER}/platform_plugin.cpp
${ADAPTORS_COMMON_FOLDER}/Controller.h
${ADAPTORS_COMMON_FOLDER}/Controller.cpp
${gmpi_ui_folder}/backends/DrawingFrameWin.h
${gmpi_ui_folder}/backends/DrawingFrameWin.cpp
${gmpi_ui_folder}/backends/DirectXGfx.h
${gmpi_ui_folder}/backends/DirectXGfx.cpp
${gmpi_ui_folder}/helpers/TimerManager.h
${gmpi_ui_folder}/helpers/TimerManager.cpp
)
#${se_sdk_hosting_folder}/ElatencyContraintType.h
#${se_sdk_hosting_folder}/gmpi_gui_hosting.h
#${se_sdk_hosting_folder}/gmpi_gui_hosting.cpp

#adelay.def
IF(WIN32)
set(srcs ${srcs}
SEVSTGUIEditorWin.h
SEVSTGUIEditorWin.cpp
${VST3_SDK}/public.sdk/source/main/dllmain.cpp
adelay.def
)
ELSE()
set(srcs ${srcs}
../se_vst3/source/SEVSTGUIEditorMac.h
../se_vst3/source/SEVSTGUIEditorMac.cpp
${se_sdk_hosting_folder}/EventHelper.h
${se_sdk_hosting_folder}/CocoaGuiHost.h
${se_sdk_hosting_folder}/CocoaGuiHost.mm
${se_sdk_hosting_folder}/SynthEditCocoaView.h
${se_sdk_hosting_folder}/SynthEditCocoaView.mm
${VST3_SDK}/public.sdk/source/main/macmain.cpp
)
ENDIF()

set(vst_gui_minimal_srcs
    ${VST3_SDK}/public.sdk/source/common/pluginview.h
    ${VST3_SDK}/public.sdk/source/common/pluginview.cpp
    ${VST3_SDK}/public.sdk/source/common/commoniids.cpp
    
    ${VST3_SDK}/public.sdk/source/vst/vstbus.h
    ${VST3_SDK}/public.sdk/source/vst/vstbus.cpp
    ${VST3_SDK}/public.sdk/source/vst/vstcomponentbase.h
    ${VST3_SDK}/public.sdk/source/vst/vstcomponentbase.cpp

    ${VST3_SDK}/public.sdk/source/vst/vsteditcontroller.h
    ${VST3_SDK}/public.sdk/source/vst/vsteditcontroller.cpp
    ${VST3_SDK}/public.sdk/source/vst/vstinitiids.cpp

    ${VST3_SDK}/public.sdk/source/vst/vstcomponent.h
    ${VST3_SDK}/public.sdk/source/vst/vstcomponent.cpp
    ${VST3_SDK}/public.sdk/source/vst/vstaudioeffect.h
    ${VST3_SDK}/public.sdk/source/vst/vstaudioeffect.cpp
    ${VST3_SDK}/public.sdk/source/vst/vstparameters.h
    ${VST3_SDK}/public.sdk/source/vst/vstparameters.cpp 
)

include_directories(
${CMAKE_CURRENT_SOURCE_DIR}
${ADAPTORS_COMMON_FOLDER}
${VST3_SDK}
${DSP_CORE}
${gmpi_sdk_folder}/Core
${gmpi_ui_folder}
${gmpi_ui_folder}/helpers
${se_sdk_hosting_folder}
${se_sdk_folder}
${se_shared_folder}
)

source_group(se_sdk FILES ${sdk_srcs})

if(APPLE)
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)
FIND_LIBRARY(QUARTZCORE_LIBRARY QuartzCore )
MARK_AS_ADVANCED (QUARTZCORE_LIBRARY)
FIND_LIBRARY(COCOA_LIBRARY Cocoa )
MARK_AS_ADVANCED (COCOA_LIBRARY)
FIND_LIBRARY(CORETEXT_LIBRARY CoreText )
MARK_AS_ADVANCED (CORETEXT_LIBRARY)
FIND_LIBRARY(OPENGL_LIBRARY OpenGL )
MARK_AS_ADVANCED (OPENGL_LIBRARY)
FIND_LIBRARY(IMAGEIO_LIBRARY ImageIO )
MARK_AS_ADVANCED (IMAGEIO_LIBRARY)
FIND_LIBRARY(COREGRAPHICS_LIBRARY CoreGraphics )
MARK_AS_ADVANCED (COREGRAPHICS_LIBRARY)
FIND_LIBRARY(ACCELERATE_LIBRARY Accelerate )
MARK_AS_ADVANCED (ACCELERATE_LIBRARY)
endif()

include (GenerateExportHeader)
# dynamic lib: add_library(${PROJECT_NAME} MODULE ${sdk_srcs} ${srcs} ${vst_gui_minimal_srcs})
add_library(${PROJECT_NAME} STATIC ${sdk_srcs} ${srcs} ${vst_gui_minimal_srcs})

# 'DEBUG' is a steinberg thing, ('_DEBUG' is standard C++)
target_compile_definitions(${PROJECT_NAME} PRIVATE 
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Debug>:DEBUG=1>
  $<$<CONFIG:Release>:DEBUG=0>
  $<$<CONFIG:Release>:NDEBUG>
)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COCOA_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${CORETEXT_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${OPENGL_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${IMAGEIO_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREGRAPHICS_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ACCELERATE_LIBRARY} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${QUARTZCORE_LIBRARY} )

#TARGET_LINK_LIBRARIES( ${PROJECT_NAME} SynthEditLib )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} base )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} pluginterfaces )

if(CMAKE_HOST_WIN32)
target_link_libraries(${PROJECT_NAME} d3d11.lib d2d1 dwrite windowscodecs)
endif()

target_compile_definitions (${PROJECT_NAME} PRIVATE -D_UNICODE -DUNICODE)

if(APPLE)
set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE TRUE)
set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION vst3)
#  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX )

  # Place module xml in bundle 'Resources' folder.
#  set(xml_path ${PROJECT_NAME}.xml)
#  target_sources(${PROJECT_NAME} PUBLIC ${xml_path})
#  set_source_files_properties(${xml_path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# windows dll extension
# set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".vst3")

########################### VST3 SDK #######################################
message(STATUS "VST3_SDK: ${VST3_SDK}")

list(APPEND CMAKE_MODULE_PATH "${VST3_SDK}/cmake/modules")

include(SMTG_VST3_SDK)

#---Add base libraries---------------------------
set(VST_SDK TRUE) # used for Base module which provides only a subset of Base for VST-SDK
add_subdirectory(${VST3_SDK}/pluginterfaces ${CMAKE_CURRENT_BINARY_DIR}/pluginterfaces)
add_subdirectory(${VST3_SDK}/base           ${CMAKE_CURRENT_BINARY_DIR}/base)

# this should be disabled on CI
#if(SE_LOCAL_BUILD)
#if(CMAKE_HOST_WIN32)
#add_custom_command(TARGET ${PROJECT_NAME}
#    # Run after all other rules within the target have been executed
#    POST_BUILD
#    COMMAND copy "$(OutDir)$(TargetName)$(TargetExt)" "C:\\Program Files\\Common Files\\VST3\\SynthEdit_VST3.vst3\\Contents\\x86_64-win"
#    COMMENT "Copy to plugin folder"
#    VERBATIM
#)
#endif()
#endif()
