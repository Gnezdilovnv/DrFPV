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

# libiio – find via vcpkg or system
find_path(LIBIIO_INCLUDE_DIR
    NAMES iio.h
    PATHS ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include
          ${CMAKE_PREFIX_PATH}/include
)

find_library(LIBIIO_LIBRARY
    NAMES iio libiio
    PATHS ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib
          ${CMAKE_PREFIX_PATH}/lib
)

if(NOT LIBIIO_INCLUDE_DIR OR NOT LIBIIO_LIBRARY)
    message(FATAL_ERROR "libiio not found. Check VCPKG_ROOT and VCPKG_TARGET_TRIPLET")
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
