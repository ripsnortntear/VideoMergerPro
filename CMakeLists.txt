cmake_minimum_required(VERSION 3.22)
project(VideoMerger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Multimedia
    MultimediaWidgets
    Network
)

add_executable(VideoMerger WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/VideoListModel.h
    src/VideoListModel.cpp
    src/FFmpegManager.h
    src/FFmpegManager.cpp
    src/MergeEngine.h
    src/MergeEngine.cpp
    src/UpdateChecker.h
    src/UpdateChecker.cpp
    app.rc
)

target_link_libraries(VideoMerger PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
)

if(MSVC)
    target_compile_options(VideoMerger PRIVATE /W4 /permissive- /utf-8)
endif()