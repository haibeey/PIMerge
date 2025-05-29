#include "LendoMerge.h"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {
  LendoMerge lendoMerge(104);

  Image down1 = create_image("files/white.jpeg");
  Image down2 = create_image("files/black.jpeg");

  // Image down1;
  // Image down2;
  // for (int i = 0; i < 3; i++) {

  //   down1 = downsample(&img1);
  //   down2 = downsample(&img2);

  //   destroy_image(&img1);
  //   destroy_image(&img2);

  //   img1 = down1;
  //   img2 = down2;
  // }

  std::vector<Image *> imgs = {&down1, &down2};

  lendoMerge.findSeam(&down1, &down2, "mask1.jpg", "mask2.jpg");
  lendoMerge.color_correct_sequence(imgs);


  char buf[100];
  for (int i = 0; i < imgs.size(); i++) {
      std::snprintf(buf,sizeof(buf),"ab%d.jpg",i);
      save_image(imgs[i], buf);
  }

  destroy_image(&down1);
  destroy_image(&down2);

  return 0;
}

// g++-14  -Iheader-files/stitcher  -I./ -L/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib -lNativeSticher  -o lendomerge LendoMerge.cpp main.cpp
// export DYLD_LIBRARY_PATH=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/macos/x86_64/lib
// ./lendomerge
