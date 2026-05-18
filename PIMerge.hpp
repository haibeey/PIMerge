#pragma once

#include "PiImage.hpp"
#include <memory>
#include <string>
#define GAMMA 2.2
#define K 0.5f
#define BRIGHTNESS 1.5
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x) * M_PI / 180.0)

#include "blending.hpp"
#include "jpeg.hpp"
#include <vector>

#ifdef __ANDROID__
#include <android/log.h>

#define LOG_TAG "MyApp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#elif defined(__APPLE__)
#include <TargetConditionals.h>
#include <iostream>
#include <sstream>

#if TARGET_OS_IPHONE
#define LOGI(...)                                                              \
  do {                                                                         \
    std::ostringstream oss;                                                    \
    oss << __VA_ARGS__;                                                        \
    std::cout << "[INFO] " << oss.str() << std::endl;                          \
  } while (0)

#endif
#endif

struct MergePoint {
  float x;
  float y;
};

struct MergeLine {
  MergePoint A;
  MergePoint B;
};

class LendoMerge {
private:
  float image_cut;
  int to_add = 0;
  int blur_strength = 100;

  void linearize(PiImageU8 &img, PiImageF &out);
  void gamma_encode(PiImageF &img, PiImageU8 &out);
  std::vector<double> compute_alpha(PiImageF &prev_img, PiImageF &curr_img);
  std::vector<double>
  compute_global_adjustment(std::vector<std::vector<double>> &alphas);
  bool merge_top_bottom(PiImageU8 &img1, PiImageU8 &img2,
                        const std::string merged_filename);
  void color_correct_sequence( std::vector<PiImageU8> &imgs);
  bool findSeam(PiImageU8 &img1, PiImageU8 &img2, PiImageU8 &mask1,
                PiImageU8 &mask2, bool is_new_img1, bool is_new_img2);
  bool merge_images_horizontal(std::vector<PiImageU8> &imgs,
                               const std::string merged_filename,
                               bool add_height = true);
  bool add_height_to(PiImageU8 &img);
  bool crop_panorama(PiImageU8 &img);
  void blur_image_helper(PiImageU8 &img, int start, int end, int blur_strength);

public:
  LendoMerge(float hfov, float camera_rotation, int blur_strength = 25);
  ~LendoMerge();

  void downsample_image(std::string image_path, std::string out_image_path,int max_width,
                        int times);
  bool merge_image_path_horizontal(std::vector<std::string> &imgs_path,
                                   std::string out_filename,
                                   bool add_height = true);
  bool merge_top_bottom_image_path(std::string image_path_1,
                                   std::string image_path_2,
                                   std::string out_filename);

  bool crop_panorama_by_path(std::string image_path, std::string out_filename);
  bool add_height_to_image_path(std::string image_path,
                                std::string out_filename);
  void blur_image(PiImageU8 &img, int start, int end);
};
