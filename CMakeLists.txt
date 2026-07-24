cmake_minimum_required(VERSION 3.16)
project(DrFPV VERSION 8.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Widgets Gui Network Concurrent REQUIRED)
find_package(Qt5TextToSpeech QUIET)
if(Qt5TextToSpeech_FOUND)
    message(STATUS "Qt5TextToSpeech found – voice alerts enabled")
    set(HAVE_TEXT_TO_SPEECH TRUE)
else()
    message(STATUS "Qt5TextToSpeech NOT found – voice alerts disabled")
    set(HAVE_TEXT_TO_SPEECH FALSE)
endif()

# libiio – use explicit paths from environment or command line
if(DEFINED LIBIIO_INCLUDE_DIR AND DEFINED LIBIIO_LIBRARY)
    set(LIBIIO_FOUND TRUE)
    message(STATUS "Using explicit libiio paths: ${LIBIIO_INCLUDE_DIR}, ${LIBIIO_LIBRARY}")
elseif(WIN32)
    # Try to find in vcpkg default location
    set(VCPKG_INSTALLED_DIR_DEFAULT "D:/a/DrFPV/DrFPV/vcpkg/installed/x64-windows")
    if(EXISTS "${VCPKG_INSTALLED_DIR_DEFAULT}/include/iio.h")
        set(LIBIIO_INCLUDE_DIR "${VCPKG_INSTALLED_DIR_DEFAULT}/include")
        set(LIBIIO_LIBRARY "${VCPKG_INSTALLED_DIR_DEFAULT}/lib/libiio.lib")
        set(LIBIIO_FOUND TRUE)
        message(STATUS "Found libiio in default vcpkg location")
    else()
        # Fallback to find_path/find_library
        find_path(LIBIIO_INCLUDE_DIR iio.h)
        find_library(LIBIIO_LIBRARY iio libiio)
        if(LIBIIO_INCLUDE_DIR AND LIBIIO_LIBRARY)
            set(LIBIIO_FOUND TRUE)
        endif()
    endif()
else()
    # Linux: use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBIIO REQUIRED libiio)
    set(LIBIIO_FOUND TRUE)
endif()

if(NOT LIBIIO_FOUND)
    message(FATAL_ERROR "libiio not found. Please provide LIBIIO_INCLUDE_DIR and LIBIIO_LIBRARY.")
endif()

set(SOURCES
    src/main.cpp
    src/SDRController.cpp
    src/SignalProcessor.cpp
    src/Visualizer.cpp
    src/RecordManager.cpp
    src/SettingsManager.cpp
    src/MainWindow.cpp
)

set(HEADERS
    src/SDRController.h
    src/SignalProcessor.h
    src/Visualizer.h
    src/RecordManager.h
    src/SettingsManager.h
    src/MainWindow.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBIIO_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Network
    Qt5::Concurrent
    ${LIBIIO_LIBRARY}
    pthread
)

if(HAVE_TEXT_TO_SPEECH)
    target_link_libraries(${PROJECT_NAME} Qt5::TextToSpeech)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_TEXT_TO_SPEECH)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
