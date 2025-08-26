cmake_minimum_required(VERSION 3.15)

project(SawDemo)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_DSP
	HAS_GUI
 FORMATS_LIST GMPI VST3
	SOURCE_FILES
	  src/SawDemo.h
	  src/SawDemo.cpp
	  src/SawDemoGui.cpp
	  src/saw-voice.h
	  src/saw-voice.cpp
)
