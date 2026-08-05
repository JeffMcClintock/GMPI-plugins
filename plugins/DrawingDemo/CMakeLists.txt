cmake_minimum_required(VERSION 3.30)

project(DrawingDemo)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP HAS_GUI
    FORMATS_LIST GMPI VST3
    SOURCE_FILES
        DrawingDemo.cpp
        DrawingDemoGui.cpp
        demo_page_gradients.h
        demo_page_shapes.h
        demo_page_text.h
)
