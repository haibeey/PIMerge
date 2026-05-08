cmake_minimum_required(VERSION 3.16)

project(PIMerge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_OS "${OS}")
set(BUILD_ARCH "${ARCH}")

include(ExternalProject)

if(NOT DEFINED BUILD_OS OR BUILD_OS STREQUAL "")
    message(FATAL_ERROR "OS is not set. Use -DOS=<platform>")
endif()

if(NOT DEFINED BUILD_ARCH OR BUILD_ARCH STREQUAL "")
    message(FATAL_ERROR "ARCH is not set. Use -DARCH=<arch>")
endif()

ExternalProject_Add(
    libjpeg_turbo_project
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/libjpeg-turbo
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/libjpeg-turbo-install
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

ExternalProject_Add(
    stitcher_project
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/stitcher
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/stitcher-install
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DLIBJPEG_TURBO_ROOT=${CMAKE_BINARY_DIR}/libjpeg-turbo-install
        -DCMAKE_C_FLAGS=-I${CMAKE_SOURCE_DIR}
        -DCMAKE_CXX_FLAGS=-I${CMAKE_SOURCE_DIR}
)

add_executable(
    ${PROJECT_NAME}
    PIMerge.cpp
    Blur.cpp
    main.cpp
)

add_dependencies(
    ${PROJECT_NAME}
    libjpeg_turbo_project
    stitcher_project
)

target_compile_options(${PROJECT_NAME} PRIVATE -O3)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/header-files/stitcher
        ${CMAKE_BINARY_DIR}/libjpeg-turbo-install/include
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_BINARY_DIR}/libjpeg-turbo-install/lib/libturbojpeg.a
        ${CMAKE_BINARY_DIR}/stitcher-install/lib/libNativeStitcher.a
)
