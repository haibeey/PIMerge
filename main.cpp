#include "LendoMerge.hpp"
#include "jpeg.h"
#include <cstdio>
#include <iostream>
#include <vector>

int main() {

  LendoMerge lendoMerge(104, 60);

  // lendoMerge.merge_image_path_horizontal(
  //     std::vector<std::string>{
  //         "files/debug/1.JPG",
  //         "files/debug/2.JPG",
  //         "files/debug/3.JPG",
  //         "files/debug/4.JPG",
  //         "files/debug/5.JPG",
  //         "files/debug/6.JPG",
  //     },
  //     "out.jpg");

  std::cout << get_cpus_count() << "\n";

  // lendoMerge.crop_panorama_by_path("out.jpg", "out_cropped.jpg");
  // LendoMerge lendoMerge(65.11, 30);

  // lendoMerge.merge_image_path_horizontal(
  //     std::vector<std::string>{
  //         "files/debug24/bottom1.jpg",
  //         "files/debug24/bottom2.jpg",
  //         "files/debug24/bottom3.jpg",
  //         "files/debug24/bottom4.jpg",
  //         "files/debug24/bottom5.jpg",
  //         "files/debug24/bottom6.jpg",
  //         "files/debug24/bottom7.jpg",
  //         "files/debug24/bottom8.jpg",
  //         "files/debug24/bottom9.jpg",
  //         "files/debug24/bottom10.jpg",
  //         "files/debug24/bottom11.jpg",
  //         "files/debug24/bottom12.jpg",
  //     },
  //     "bottom.jpg", false);

  // lendoMerge.merge_image_path_horizontal(
  //     std::vector<std::string>{
  //         "files/debug24/top1.jpg",
  //         "files/debug24/top2.jpg",
  //         "files/debug24/top3.jpg",
  //         "files/debug24/top4.jpg",
  //         "files/debug24/top5.jpg",
  //         "files/debug24/top6.jpg",
  //         "files/debug24/top7.jpg",
  //         "files/debug24/top8.jpg",
  //         "files/debug24/top9.jpg",
  //         "files/debug24/top10.jpg",
  //         "files/debug24/top11.jpg",
  //         "files/debug24/top12.jpg",
  //     },
  //     "top.jpg", false);

  // lendoMerge.merge_top_bottom_image_path("top.jpg", "bottom.jpg",
  //                                        "final_result.jpg");

  // lendoMerge.crop_panorama_by_path("final_result.jpg", "out_cropped.jpg");
  //
  //
  // LendoMerge lendoMerge(65.11, 30);

  // lendoMerge.merge_image_path_horizontal(
  //     std::vector<std::string>{
  //         "files/debug/bottom1.jpg",
  //         "files/debug/bottom2.jpg",
  //         "files/debug/bottom3.jpg",
  //         "files/debug/bottom4.jpg",
  //         "files/debug/bottom5.jpg",
  //         "files/debug/bottom6.jpg",
  //         "files/debug/bottom7.jpg",
  //         "files/debug/bottom8.jpg",
  //         "files/debug/bottom9.jpg",
  //         "files/debug/bottom10.jpg",
  //         "files/debug/bottom11.jpg",
  //         "files/debug/bottom12.jpg",
  //     },
  //     "bottom.jpg", false);
  //
  //
  Image img = create_image("files/debug/1.JPG");
  std::cout << img.height << std::endl;
  lendoMerge.blur_image(&img, 0, 100);
  save_image(&img, "out.jpg");
  destroy_image(&img);



  return 0;
}
