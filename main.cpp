#include "LendoMerge.h"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {
  LendoMerge lendoMerge(104);

  lendoMerge.downsample_image("files/1.jpg", "down1.jpg");
  lendoMerge.downsample_image("files/2.jpg", "down2.jpg");

  lendoMerge.merge_two_by_image_path("down1.jpg", "down2.jpg", "out.jpg");
  Image down1 = create_image("down1.jpg");
  Image down2 = create_image("down2.jpg");

  lendoMerge.merge(std::vector<std::string>{"result1.jpg", "result2.jpg"},
                   down1.width);

  destroy_image(&down1);
  destroy_image(&down2);

  return 0;
}

// g++-14  -Iheader-files/stitcher  -I./
// -L/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// -lNativeStitcher  -o lendomerge LendoMerge.cpp main.cpp export
// DYLD_LIBRARY_PATH=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// ./lendomerge
