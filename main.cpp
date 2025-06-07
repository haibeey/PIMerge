#include "LendoMerge.h"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {
  LendoMerge lendoMerge(104);

  lendoMerge.merge_six_by_image_path(
      std::vector<std::string>{
          "files/debug/1.JPG",
          "files/debug/2.JPG",
          "files/debug/3.JPG",
          "files/debug/4.JPG",
          "files/debug/5.JPG",
          "files/debug/6.JPG",
      },
      "out.jpg");



  return 0;
}

// g++-14  -Iheader-files/stitcher  -I./
// -L/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// -lNativeStitcher  -o lendomerge LendoMerge.cpp main.cpp export
// DYLD_LIBRARY_PATH=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// ./lendomerge
