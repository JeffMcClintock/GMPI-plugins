cmake_minimum_required(VERSION 3.15)

project(SawDemo)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_DSP
	HAS_GUI
 FORMATS_LIST GMPI VST3
	SOURCE_FILES
	  src/gmpi-saw-demo.h
	  src/gmpi-saw-demo.cpp
	  src/gmpi-saw-demo-editor.cpp
	  src/saw-voice.h
	  src/saw-voice.cpp
)
