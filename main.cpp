#include "LendoMerge.hpp"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {

  LendoMerge lendoMerge(104, 60);

  lendoMerge.merge_image_path_horizontal(
      std::vector<std::string>{
          "files/debug/1.JPG",
          "files/debug/2.JPG",
          "files/debug/3.JPG",
          "files/debug/4.JPG",
          "files/debug/5.JPG",
          "files/debug/6.JPG",
      },
      "out.jpg");



  // Image img = create_image("files/debug/1.JPG");
  // // std::cout << img.height << std::endl;
  // lendoMerge.blur_image(&img, 0, 100);
  // lendoMerge.blur_image(&img, img.height - 100, img.height);

  // save_image(&img, "out1.jpg");
  // destroy_image(&img);

  return 0;
}
