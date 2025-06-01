#include "LendoMerge.h"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {
  LendoMerge lendoMerge(104);

  Image img1 = create_image("files/2.jpg");
  Image img2 = create_image("files/3.jpg");

  Image down1, down2;
  for (int i = 0; i < 3; i++) {
    down1 = downsample(&img1);
    down2 = downsample(&img2);

    destroy_image(&img1);
    destroy_image(&img2);

    img1 = down1, img2 = down2;
  }

  // lendoMerge.merge_two(&down1, &down2, "result2.jpg");
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
