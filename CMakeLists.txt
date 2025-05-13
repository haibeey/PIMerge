cmake_minimum_required(VERSION 2.8.12...3.28)

project(LendoMerge LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pull in your stitcher subproject; it must define a target called NativeStitcher
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/stitcher" stitcher-build)

# Now define your executable (note the exact same name “LendoMerge”)
add_executable(LendoMerge
    LendoMerge.cpp
)

# Link against the stitcher library target
target_link_libraries(LendoMerge
    PRIVATE
      NativeStitcher
)
