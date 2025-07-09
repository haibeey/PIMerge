#include "LendoMerge.hpp"
#include "jpeg.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>
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

  return 0;
}
