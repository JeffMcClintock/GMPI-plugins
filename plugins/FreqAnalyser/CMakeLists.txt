cmake_minimum_required(VERSION 3.15)

project(FreqAnalyser)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_DSP
	HAS_GUI
 FORMATS_LIST GMPI CLAP
	SOURCE_FILES
		FreqAnalyser.cpp
		FreqAnalyserGui.cpp
		real_fft.h
		real_fft.cpp
		${gmpi_ui_folder}/helpers/Timer.h
		${gmpi_ui_folder}/helpers/Timer.cpp
)
