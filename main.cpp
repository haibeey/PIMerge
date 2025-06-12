#include "LendoMerge.hpp"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {
  LendoMerge lendoMerge(65.11,30);
  // LendoMerge lendoMerge(104,60);

  lendoMerge.merge_by_image_path(
      std::vector<std::string>{
          "files/debug24/bottom1.jpg",
          "files/debug24/bottom2.jpg",
          "files/debug24/bottom3.jpg",
          "files/debug24/bottom4.jpg",
          "files/debug24/bottom5.jpg",
          "files/debug24/bottom6.jpg",
          "files/debug24/bottom7.jpg",
          "files/debug24/bottom8.jpg",
          "files/debug24/bottom9.jpg",
          "files/debug24/bottom10.jpg",
          "files/debug24/bottom11.jpg",
          "files/debug24/bottom12.jpg",
      },
      "out.jpg");



  return 0;
}

// g++-14  -Iheader-files/stitcher  -I./
// -L/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// -lNativeStitcher  -o lendomerge LendoMerge.cpp main.cpp export
// DYLD_LIBRARY_PATH=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// ./lendomerge
