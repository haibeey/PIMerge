cmake_minimum_required(VERSION 2.8.12...3.28)

project(LendoMerge LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_OS      "${OS}")
set(BUILD_ARCH      "${ARCH}")

if(NOT DEFINED BUILD_OS OR BUILD_OS STREQUAL "")
    message(FATAL_ERROR "OS is not set. Please set the OS environment variable. set with -DOS=mac etc")
endif()

if(NOT DEFINED BUILD_ARCH OR BUILD_ARCH STREQUAL "")
    message(FATAL_ERROR "ARCH is not set. Please set the ARCH environment variable. et with -DARCH=x86_64 etc")
endif()
set(STITCHER_PATH "${CMAKE_SOURCE_DIR}/libs/stitcher/${BUILD_OS}/${BUILD_ARCH}")
set(LIBTURBOJPEG_PATH "${CMAKE_SOURCE_DIR}/libs/libturbojpeg/${BUILD_OS}/${BUILD_ARCH}")

if(CMAKE_OS)
    message(FATAL_ERROR
      "CMAKE_INSTALL_PREFIX not set. "
      "Please provide a prefix using -DCMAKE_INSTALL_PREFIX=<path>")
endif()

include_directories(${CMAKE_SOURCE_DIR}/header-files/stitcher)
link_directories(${STITCHER_PATH} ${LIBTURBOJPEG_PATH})

add_executable(LendoMerge LendoMerge.cpp main.cpp)
target_link_libraries(LendoMerge PRIVATE NativeStitcher turbojpeg)
