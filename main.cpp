#include "LendoMerge.hpp"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {

  // LendoMerge lendoMerge(104,60);

  // lendoMerge.merge_by_image_path(
  //     std::vector<std::string>{
  //         "files/debug/1.JPG",
  //         "files/debug/2.JPG",
  //         "files/debug/3.JPG",
  //         "files/debug/4.JPG",
  //         "files/debug/5.JPG",
  //         "files/debug/6.JPG",
  //     },
  //     "out.jpg");

  LendoMerge lendoMerge(65.11, 30);

  lendoMerge.merge_image_path_horizontal(
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
      "bottom.jpg", false);

  lendoMerge.merge_image_path_horizontal(
      std::vector<std::string>{
          "files/debug24/top1.jpg",
          "files/debug24/top2.jpg",
          "files/debug24/top3.jpg",
          "files/debug24/top4.jpg",
          "files/debug24/top5.jpg",
          "files/debug24/top6.jpg",
          "files/debug24/top7.jpg",
          "files/debug24/top8.jpg",
          "files/debug24/top9.jpg",
          "files/debug24/top10.jpg",
          "files/debug24/top11.jpg",
          "files/debug24/top12.jpg",
      },
      "top.jpg", false);

  lendoMerge.merge_top_bottom_image_path("top.jpg","bottom.jpg","final_result.jpg");

  // Image img1 = create_image("files/debug24/top1.jpg");
  // Image img2 = create_image("files/debug24/bottom1.jpg");



  return 0;
}

// g++-14  -Iheader-files/stitcher  -I./
// -L/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// -lNativeStitcher  -o lendomerge LendoMerge.cpp main.cpp export
// DYLD_LIBRARY_PATH=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// ./lendomerge
