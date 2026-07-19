cmake_minimum_required(VERSION 4.0.0)
project(WfProfile VERSION 0.1 LANGUAGES CXX)

# vcpkg only on Windows
if(WIN32)
    if(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_ROOT $ENV{VCPKG_ROOT})
    else()
        set(VCPKG_ROOT "C:/Users/Windows/.vcpkg-clion/vcpkg")
    endif()
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core dependencies
find_package(CURL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Concurrent WebEngineWidgets)
find_package(nlohmann_json CONFIG REQUIRED)

# LibLZMA: try imported target, else plain find
if(NOT TARGET LibLZMA::LibLZMA)
    find_package(LibLZMA)
endif()
if(NOT TARGET LibLZMA::LibLZMA)
    find_library(LIBLZMA_LIBRARY NAMES lzma)
    find_path(LIBLZMA_INCLUDE_DIR NAMES lzma.h)
    if(NOT LIBLZMA_LIBRARY OR NOT LIBLZMA_INCLUDE_DIR)
        message(FATAL_ERROR "LibLZMA not found")
    endif()
endif()

# Tesseract: try imported target, else plain find
if(NOT TARGET Tesseract::libtesseract)
    find_package(Tesseract CONFIG)
endif()
if(NOT TARGET Tesseract::libtesseract)
    find_library(TESSERACT_LIBRARY NAMES tesseract)
    find_path(TESSERACT_INCLUDE_DIR NAMES tesseract/baseapi.h)
    if(NOT TESSERACT_LIBRARY OR NOT TESSERACT_INCLUDE_DIR)
        message(FATAL_ERROR "Tesseract not found")
    endif()
endif()

# Source files
set(PROJECT_SOURCES
        src/window/main.cpp
        src/window/mainwindow.cpp
        src/window/mainwindow.h
        src/window/mainwindow.ui
        src/apiParser/apiParser.h
        src/apiParser/apiParser.cpp
        src/dataReader/dataReader.h
        src/dataReader/dataReader.cpp
        src/FileAccess/FileAccess.h
        src/FileAccess/FileAccess.cpp
        src/background/BackgroundWorker.cpp
        src/background/BackgroundWorker.h
        src/autostart/autostart.h
        src/window/overviewPartWidget.cpp
        src/window/overviewPartWidget.h
        src/EELogParser/logParser.cpp
        src/EELogParser/logParser.h
        src/window/CaptureOverlay.cpp
        src/window/CaptureOverlay.h
)

# Platform-specific sources
set(PLATFORM_SOURCES "")
if(WIN32)
    list(APPEND PLATFORM_SOURCES
            src/autostart/AutoStartWindows.cpp
    )
    add_definitions(-DWINDOWS_PLATFORM)
elseif(UNIX AND NOT APPLE)
    list(APPEND PLATFORM_SOURCES
            src/autostart/autoStartLinux.cpp
    )
    add_definitions(-DLINUX_PLATFORM)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

list(APPEND PROJECT_SOURCES ${PLATFORM_SOURCES})

# Executable
qt_add_executable(WfProfile MANUAL_FINALIZATION ${PROJECT_SOURCES})

# Include directories
target_include_directories(WfProfile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(WfProfile PRIVATE ${CURL_INCLUDE_DIRS})

if(NOT TARGET LibLZMA::LibLZMA)
    target_include_directories(WfProfile PRIVATE ${LIBLZMA_INCLUDE_DIR})
endif()

if(NOT TARGET Tesseract::libtesseract)
    target_include_directories(WfProfile PRIVATE ${TESSERACT_INCLUDE_DIR})
endif()

# Link libraries
target_link_libraries(WfProfile PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        Qt6::Concurrent
        Qt6::WebEngineWidgets
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

if(TARGET LibLZMA::LibLZMA)
    target_link_libraries(WfProfile PRIVATE LibLZMA::LibLZMA)
else()
    target_link_libraries(WfProfile PRIVATE ${LIBLZMA_LIBRARY})
endif()

if(TARGET Tesseract::libtesseract)
    target_link_libraries(WfProfile PRIVATE Tesseract::libtesseract)
else()
    target_link_libraries(WfProfile PRIVATE ${TESSERACT_LIBRARY})
endif()

if(WIN32)
    target_link_libraries(WfProfile PRIVATE psapi)
endif()

qt_finalize_executable(WfProfile)

set_target_properties(WfProfile PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)