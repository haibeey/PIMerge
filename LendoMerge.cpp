
#include "simde/simde/x86/avx2.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "LendoMerge.hpp"

LendoMerge::~LendoMerge() {
  if (map_x != nullptr)
    destroy_image_f(map_x.get());
  if (map_y != nullptr)
    destroy_image_f(map_y.get());
}

bool LendoMerge::findSeam(Image *img1, Image *img2, Image *mask1,
                          Image *mask2) {

  assert(img1->width == img2->width && img1->height == img2->height);

  int err_width = static_cast<int>(img1->width * IMAGE_CUT);
  int start = static_cast<int>(img1->width * (1 - IMAGE_CUT));

  std::vector<std::vector<short>> E(img1->height,
                                    std::vector<short>(err_width));

  for (int i = 0; i < img1->height; i++) {
    for (int j = 0; j < err_width; j++) {
      unsigned char a = img1->data[(i * img1->width) + start + j];
      unsigned char b = img2->data[(i * img2->width) + j];
      E[i][j] = (b - a) * (b - a);
    }
  }

  std::vector<std::vector<short>> dp(img1->height,
                                     std::vector<short>(err_width));
  for (int i = 0; i < err_width; i++) {
    dp[0][i] = E[0][i];
  }

  for (int i = 1; i < img1->height; i++) {
    for (int j = 0; j < err_width; j++) {
      int a, b, c;
      c = b = a = 1 << 31;

      if (j - 1 >= 0) {
        a = dp[i - 1][j - 1];
      }
      b = dp[i - 1][j];
      if (j + 1 < err_width) {
        c = dp[i - 1][j + 1];
      }

      dp[i][j] = E[i][j] + std::min(a, std::min(b, c));
    }
  }

  E.clear();

  auto min_element = std::min_element(dp[img1->height - 1].begin(),
                                      dp[img1->height - 1].end());
  int index = static_cast<int>(
      std::distance(dp[img1->height - 1].begin(), min_element));

  std::vector<int> path;
  path.push_back(index);

  for (int i = img1->height - 2; i >= 0; i--) {
    int a, b, c;
    c = b = a = 1 << 31;

    if (index - 1 >= 0) {
      a = dp[i][index - 1];
    }
    b = dp[i][index];
    if (index + 1 < err_width) {
      c = dp[i][index + 1];
    }

    if (a <= b && a <= c && index - 1 >= 0) {
      index = index - 1;
    } else if (c <= a && c <= b && index + 1 < err_width) {
      index = index + 1;
    }

    path.push_back(index);
  }

  dp.clear();

  for (int i = img1->height - 1; i >= 0; i--) {
    int a = path[i];

    int s = (i * img1->width) + start;
    int m = (i * img1->width) + start + a;
    int e = (i * img1->width) + img1->width;

    std::memset(mask1->data + s, 255, m - s);
    std::memset(mask1->data + m, 0, e - m);

    s = (i * img1->width);
    m = (i * img1->width) + a;

    std::memset(mask2->data + s, 0, m - s);
    std::memset(mask2->data + m, 255, e - m);
  }

  return true;
}

void LendoMerge::linearize(Image *img, ImageF *out) {
  assert(img->width == out->width && img->height == out->height &&
         img->channels == out->channels);
  for (int i = 0; i < (img->width * img->height * out->channels); i++) {
    out->data[i] = std::pow((img->data[i] / 255.0), GAMMA);
  }
}

void LendoMerge::gamma_encode(ImageF *img, Image *out) {
  assert(img->width == out->width && img->height == out->height &&
         img->channels == out->channels);

  float gamma_inv = 1.0 / GAMMA;
  for (int i = 0; i < (img->width * img->height * out->channels); i++) {
    float v = std::pow(img->data[i], gamma_inv) * 255.0;
    unsigned char p = static_cast<unsigned char>(v);
    if (v < 0) {
      p = 0;
    } else if (v > 255) {
      p = 255;
    }
    out->data[i] = p;
  }
}

std::vector<double> LendoMerge::compute_alpha(ImageF *prev_img,
                                              ImageF *curr_img) {

  int overlap_width = static_cast<int>(prev_img->width * IMAGE_CUT);
  int start_overlap_width = static_cast<int>(prev_img->width * (1 - IMAGE_CUT));

  std::vector<double> sums_prev = std::vector<double>(3);
  std::vector<double> sums_cur = std::vector<double>(3);

  for (int channel = 0; channel < prev_img->channels; channel++) {
    for (int i = 0; i < prev_img->height; i++) {
      for (int j = 0; j < overlap_width; j++) {
        int cur_pos =
            (i * prev_img->width) + ((j + channel) * prev_img->channels);
        int prev_pos =
            (i * prev_img->width) +
            ((j + start_overlap_width + channel) * prev_img->channels);

        sums_prev[channel] += prev_img->data[prev_pos];
        sums_cur[channel] += curr_img->data[cur_pos];
      }
    }
  }

  std::vector<double> result;
  for (int i = 0; i < sums_prev.size(); i++) {
    result.push_back(sums_prev[i] / (sums_cur[i] + 1e-8));
  }

  return result;
}

std::vector<double>
LendoMerge::compute_global_adjustment(std::vector<std::vector<double>> alphas) {

  std::vector<double> result = std::vector<double>(3);
  std::vector<double> alphas_sq = std::vector<double>(3);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < alphas.size(); j++) {
      result[i] += alphas[j][i];
      alphas_sq[i] += std::pow(alphas[j][i], 2.0);
    }
  }

  for (int i = 0; i < 3; i++) {
    result[i] = result[i] / alphas_sq[i];
  }

  return result;
}

void LendoMerge::color_correct_sequence(const std::vector<Image *> &imgs) {
  std::vector<ImageF> linearize_images;
  for (int i = 0; i < imgs.size(); i++) {
    linearize_images.push_back(create_empty_image_f(
        imgs[i]->width, imgs[i]->height, imgs[i]->channels));
    linearize(imgs[i], &linearize_images[i]);
  }

  std::vector<std::vector<double>> alphas;
  alphas.push_back({1.0, 1.0, 1.0});
  for (int i = 1; i < imgs.size(); i++)
    alphas.push_back(
        compute_alpha(&linearize_images[i - 1], &linearize_images[i]));

  std::vector<double> g = compute_global_adjustment(alphas);



  for (int i = 0; i < imgs.size(); i++) {
    std::vector<double> combined(3);
    for (int c = 0; c < 3; c++) {
      double gain = g[c] * alphas[i][c];
      combined[c] = std::pow(gain, 1.0 / GAMMA) * BRIGHTNESS;
    }

    const int total = imgs[i]->width * imgs[i]->height * imgs[i]->channels;
    for (int k = 0; k < total; k++) {
      linearize_images[i].data[k] *= combined[k % 3];
    }
    gamma_encode(&linearize_images[i], imgs[i]);
  }

  for (auto &imgf : linearize_images) {
    destroy_image_f(&imgf);
  }
}

void LendoMerge::compute_map(int width, int height, int channels) {

  if (map_x != nullptr)
    destroy_image_f(map_x.get());
  if (map_y != nullptr)
    destroy_image_f(map_y.get());

  map_x =
      std::make_unique<ImageF>(create_empty_image_f(width, height, channels));
  map_y =
      std::make_unique<ImageF>(create_empty_image_f(width, height, channels));

  simde__m256 eight = simde_mm256_set1_ps(8.0f);
  simde__m256 one = simde_mm256_set1_ps(1.0f);
  simde__m256 ws = simde_mm256_set1_ps((float)width);
  simde__m256 hs = simde_mm256_set1_ps((float)height);

  simde__m256 b =
      simde_mm256_setr_ps(0.0f, 0.0f, 0.0f, 0.0, 0.0f, 0.0f, 0.0f, 0.0);

  for (int y = 0; y < height; y++) {
    simde__m256 a =
        simde_mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);

    float *map_x_src = map_x->data + (y * width * channels);
    float *map_y_src = map_y->data + (y * width * channels);
    int x = 0;
    for (; x < (width - 8); x += 8) {
      simde_mm256_storeu_ps(map_x_src, a);
      simde_mm256_storeu_ps(map_y_src, b);
      a = simde_mm256_add_ps(a, eight);

      map_x_src += 8;
      map_y_src += 8;
    }
    for (; x < width; x++) {
      map_x_src[0] = static_cast<float>(x);
      map_y_src[0] = static_cast<float>(y);
      ++map_x_src;
      ++map_y_src;
    }

    b = simde_mm256_add_ps(b, one);
  }

  // x = (2 * x_indices - w) / w
  // y = (2 * y_indices - h) / h
  simde__m256 two = simde_mm256_set1_ps(2.0f);
  for (int y = 0; y < height; y++) {
    float *map_x_src = map_x->data + (y * width * channels);
    float *map_y_src = map_y->data + (y * width * channels);
    int x = 0;
    for (; x < (width - 8); x += 8) {
      simde__m256 vec_a = simde_mm256_loadu_ps(map_x_src);
      simde__m256 vec_b = simde_mm256_loadu_ps(map_y_src);

      vec_a = simde_mm256_mul_ps(vec_a, two);
      vec_b = simde_mm256_mul_ps(vec_b, two);

      vec_a = simde_mm256_sub_ps(vec_a, ws);
      vec_a = simde_mm256_div_ps(vec_a, ws);

      vec_b = simde_mm256_sub_ps(vec_b, hs);
      vec_b = simde_mm256_div_ps(vec_b, hs);

      simde_mm256_storeu_ps(map_x_src, vec_a);
      simde_mm256_storeu_ps(map_y_src, vec_b);

      map_x_src += 8;
      map_y_src += 8;
    }

    for (; x < width; x++) {
      map_x_src[0] = (2.0f * map_x_src[0] - width) / static_cast<float>(width);
      map_y_src[0] =
          (2.0f * map_y_src[0] - height) / static_cast<float>(height);
      ++map_x_src;
      ++map_y_src;
    }
  }

  // r = x**2 + y**2
  ImageF r = create_empty_image_f(width, height, channels);
  for (int y = 0; y < height; y++) {
    float *map_x_src = map_x->data + (y * width * channels);
    float *map_y_src = map_y->data + (y * width * channels);
    float *r_src = r.data + (y * width * channels);
    int x = 0;
    for (; x < (width - 8); x += 8) {
      simde__m256 vec_a = simde_mm256_loadu_ps(map_x_src);
      simde__m256 vec_b = simde_mm256_loadu_ps(map_y_src);

      simde_mm256_storeu_ps(
          r_src, simde_mm256_add_ps(simde_mm256_mul_ps(vec_a, vec_a),
                                    simde_mm256_mul_ps(vec_b, vec_b)));

      map_x_src += 8;
      map_y_src += 8;
      r_src += 8;
    }

    for (; x < width; x++) {
      r_src[0] = (map_x_src[0] * map_x_src[0]) + (map_y_src[0] * map_y_src[0]);
      ++r_src;
      ++map_x_src;
      ++map_y_src;
    }
  }

  // x_distorted = x * (1 + k * r)
  // y_distorted = y * (1 + k * r)
  simde__m256 k = simde_mm256_set1_ps(K);
  for (int y = 0; y < height; y++) {
    float *map_x_src = map_x->data + (y * width * channels);
    float *map_y_src = map_y->data + (y * width * channels);
    float *r_src = r.data + (y * width * channels);
    int x = 0;
    for (; x < (width - 8); x += 8) {
      simde__m256 vec_a = simde_mm256_loadu_ps(map_x_src);
      simde__m256 vec_b = simde_mm256_loadu_ps(map_y_src);
      simde__m256 vec_r = simde_mm256_loadu_ps(r_src);

      simde__m256 rk = simde_mm256_add_ps(simde_mm256_mul_ps(k, vec_r), one);

      simde_mm256_storeu_ps(map_x_src, simde_mm256_mul_ps(vec_a, rk));
      simde_mm256_storeu_ps(map_y_src, simde_mm256_mul_ps(vec_b, rk));

      map_x_src += 8;
      map_y_src += 8;
      r_src += 8;
    }

    for (; x < width; x++) {
      map_x_src[0] = map_x_src[0] * (1 + (K * r_src[0]));
      map_y_src[0] = map_y_src[0] * (1 + (K * r_src[0]));
      ++r_src;
      ++map_x_src;
      ++map_y_src;
    }
  }

  destroy_image_f(&r);

  // map_x = ((x_distorted + 1) * w) / 2
  // map_y = ((y_distorted + 1) * h) / 2

  for (int y = 0; y < height; y++) {
    float *map_x_src = map_x->data + (y * width * channels);
    float *map_y_src = map_y->data + (y * width * channels);
    int x = 0;
    for (; x < (width - 8); x += 8) {
      simde__m256 vec_a = simde_mm256_loadu_ps(map_x_src);
      simde__m256 vec_b = simde_mm256_loadu_ps(map_y_src);

      simde_mm256_storeu_ps(
          map_x_src,
          simde_mm256_div_ps(
              simde_mm256_mul_ps(simde_mm256_add_ps(vec_a, one), ws), two));
      simde_mm256_storeu_ps(
          map_y_src,
          simde_mm256_div_ps(
              simde_mm256_mul_ps(simde_mm256_add_ps(vec_b, one), hs), two));

      map_x_src += 8;
      map_y_src += 8;
    }

    for (; x < width; x++) {
      map_x_src[0] = ((map_x_src[0] + 1) * width) / 2.0f;
      map_y_src[0] = ((map_y_src[0] + 1) * height) / 2.0f;
      ++map_x_src;
      ++map_y_src;
    }
  }
}

void LendoMerge::bilinear_interpolate(Image *img) {

  assert(img->channels == RGB_CHANNELS);

  if (map_x == nullptr || map_y == nullptr) {
    compute_map(img->width, img->height, GRAY_CHANNELS);
  }

  Image out = create_empty_image(img->width, img->height, img->channels);
  const int W = img->width;
  const int H = img->height;
  const int C = img->channels;

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      int map_pos = y * W + x;
      float i = map_x->data[map_pos];
      float j = map_y->data[map_pos];

      if (i < 0.0f || i >= W - 1 || j < 0.0f || j >= H - 1)
        continue;

      int x0 = static_cast<int>(std::floor(i));
      int y0 = static_cast<int>(std::floor(j));
      float dx = i - x0;
      float dy = j - y0;

      for (int c = 0; c < C; c++) {
        int base11 = ((y0)*W + (x0)) * C + c;
        int base21 = ((y0)*W + (x0 + 1)) * C + c;
        int base12 = ((y0 + 1) * W + (x0)) * C + c;
        int base22 = ((y0 + 1) * W + (x0 + 1)) * C + c;

        float Q11 = img->data[base11];
        float Q21 = img->data[base21];
        float Q12 = img->data[base12];
        float Q22 = img->data[base22];

        float pixel = Q11 * (1 - dx) * (1 - dy) + Q21 * (dx) * (1 - dy) +
                      Q12 * (1 - dx) * (dy) + Q22 * (dx) * (dy);

        int out_idx = map_pos * C + c;
        out.data[out_idx] = clamp(static_cast<int>(std::round(pixel)), 0, 255);
      }
    }
  }

  destroy_image(img);
  img->data = out.data;
}

bool LendoMerge::merge_two(Image *img1, Image *img2,
                           const char *merged_filename) {

  bool result = false;
  Image mask1 = convert_RGB_to_gray(img1);
  Image mask2 = convert_RGB_to_gray(img2);
  if (!findSeam(img1, img2, &mask1, &mask2))
    return false;

  int bands = 5;

  int out_width = (img1->width * 2) - static_cast<int>(img1->width * IMAGE_CUT);

  StitchRect out_size = {0, 0, out_width, img1->height};

  Blender *b = create_blender(MULTIBAND, out_size, bands);

  feed(b, img1, &mask1, StitchPoint{0, 0});
  feed(b, img2, &mask2,
       StitchPoint{
           img1->width - (2 * static_cast<int>(img1->width * IMAGE_CUT)), 0});
  blend(b);

  destroy_image(&mask1);
  destroy_image(&mask2);

  if (b->result.data != NULL) {
    int right_cut = 0;
    for (int i = (b->result.width * RGB_CHANNELS) - 1; i > 100; i--) {
      if (b->result.data[i] > 0) {
        right_cut = b->result.width - (i / RGB_CHANNELS);
        break;
      }
    }
    crop_image(&b->result, 0, 0, 0, right_cut);
    if (!save_image(&b->result, merged_filename)) {
      result = false;
      goto clean;
    }
    result = true;
    goto clean;
  }

clean:
  destroy_blender(b);
  return result;
}

std::string LendoMerge::merge(std::vector<std::string> imgs_path, int img_width,
                              std::string result) {
  // img_width holds the with of each image , assuming all images have the same
  // size assert(imgs_path.size() % 6 == 0);
  int total_width = 0;
  int max_height = 0;

  for (std::string image_path : imgs_path) {
    Image img = (create_image(image_path.c_str()));
    total_width += img.width - (img_width / 2);
    max_height = max(max_height, img.height);
    destroy_image(&img);
  }

  StitchRect out_size = {0, 0, total_width, max_height};

  Blender *b = create_blender(FEATHER, out_size, -1);

  int x = 0;
  int x_point = 0;
  int l = 0, r = 1;
  for (std::string image_path : imgs_path) {
    Image img = (create_image(image_path.c_str()));
    float cut = static_cast<float>(img_width) / 2.0f;
    Image mask;

    if (x > 0) {
      l = 1;
      r = 0;
    }

    mask = create_image_mask(img.width, img.height,
                             cut / static_cast<float>(img.width), l, r);

    feed(b, &img, &mask, StitchPoint{x_point, 0});

    x_point += img.width - img_width;

    destroy_image(&img);
    destroy_image(&mask);
    x++;
  }

  blend(b);
  if (b->result.data != NULL) {
    if (!save_image(&b->result, result.c_str())) {
      result = false;
      goto clean;
    }
    result = true;
    goto clean;
  }

clean:
  destroy_blender(b);
  return result;
}

void LendoMerge::downsample_image(std::string image_path,
                                  std::string out_image_path) {
  Image img = create_image(image_path.c_str());
  Image down;
  int x = 2;
  while (img.width > 300 && x > 0) {
    down = downsample(&img);
    destroy_image(&img);
    img = down;
    x--;
  }
  save_image(&img, out_image_path.c_str());
  destroy_image(&img);
}

bool LendoMerge::merge_two_by_image_path(std::string image_path_1,
                                         std::string image_path_2,
                                         std::string out_filename) {
  Image img1 = create_image(image_path_1.c_str());
  Image img2 = create_image(image_path_2.c_str());

  color_correct_sequence(std::vector<Image *>{&img1, &img2});

  bool result = merge_two(&img1, &img2, out_filename.c_str());

  destroy_image(&img1);
  destroy_image(&img2);

  return result;
}

void LendoMerge::add_height(Image *img) {
  int new_width = img->width;
  int new_height = img->height;
  int to_add = 0;
  if (img->height < new_width / 2) {
    to_add = (new_width / 2) - new_height;
    new_height += to_add;
  }

  Image new_img = create_empty_image(new_width, new_height, img->channels);
  int half_to_add = to_add / 2;

  for (int y = 0; y < half_to_add; y++) {
      unsigned char *image_start =img->data + ((half_to_add - y) * img->width * new_img.channels);
      unsigned char *new_image_start =new_img.data + (y * img->width * new_img.channels);
      memcpy(new_image_start, image_start, new_img.width * new_img.channels);
  }

  int yy = 0;
  for (int y = (half_to_add); y < new_height - (half_to_add) && yy < img->height; y++) {

    int x = 0;
    unsigned char *new_image_start =
        new_img.data + (y * new_img.width * new_img.channels);
    unsigned char *image_start =
        img->data + (yy * img->width * new_img.channels);
    memcpy(new_image_start, image_start, new_img.width * new_img.channels);
    yy++;
  }

  for (int y = 0; y < half_to_add; y++) {
      unsigned char *image_start =img->data + ((img->height  - y - 1) * img->width * new_img.channels);
      unsigned char *new_image_start =new_img.data + ((img->height + half_to_add + y) * img->width * new_img.channels);
      memcpy(new_image_start, image_start, new_img.width * new_img.channels);
  }

  free(img->data);
  img->data = new_img.data;
  img->width = new_width;
  img->height = new_height;
}

bool LendoMerge::merge_six(std::vector<Image *> imgs,
                           const char *merged_filename) {

  assert(imgs.size() > 0 && imgs.size() % 6 == 0);
  color_correct_sequence(imgs);
  bool result = false;
  int bands = 5;
  int out_width = 0;
  std::vector<Image> masks(imgs.size());
  std::vector<int> x_points(imgs.size());
  for (int i = 0; i < imgs.size(); i++) {
    masks[i].width = -1;
  }
  x_points[0] = 0;
  Blender *b = nullptr;
  StitchRect out_size;

  int gap = (1 << bands);

  for (int i = 0; i < imgs.size() - 1; i++) {
    Image mask1 = convert_RGB_to_gray(imgs[i]);
    std::memset(mask1.data, 255, mask1.channels * mask1.width * mask1.height);
    Image mask2 = convert_RGB_to_gray(imgs[i + 1]);
    std::memset(mask2.data, 255, mask2.channels * mask2.width * mask2.height);

    if (!findSeam(imgs[i], imgs[i + 1], &mask1, &mask2))
      goto clean;

    int mul = 1;
    if (i == 0) {
      mul = 2;
    }
    masks[i] = mask1;
    masks[i + 1] = mask2;
    out_width +=
        (mask1.width * mul) - static_cast<int>(mask1.width * IMAGE_CUT);

    x_points[i + 1] = x_points[i] + mask2.width -
                      static_cast<int>(mask1.width * IMAGE_CUT) - gap;
  }

  out_size = {0, 0, out_width, imgs[0]->height};
  b = create_blender(MULTIBAND, out_size, bands);

  for (int i = 0; i < imgs.size(); i++) {
    feed(b, imgs[i], &masks[i], StitchPoint{x_points[i], 0});
  }

  blend(b);

  if (b->result.data != NULL) {
    int right_cut = 0;
    for (int i = (b->result.width * RGB_CHANNELS) - 1; i > 200; i--) {
      if (b->result.data[i] > 0) {
        right_cut = b->result.width - (i / RGB_CHANNELS);
        break;
      }
    }
    int join = (b->result.width - right_cut) -
               static_cast<int>((b->result.width - right_cut) * 0.97f);
    crop_image(&b->result, 0, 0, 0, right_cut + join);
    add_height(&b->result);
    if (!save_image(&b->result, merged_filename)) {
      result = false;
      goto clean;
    }
    result = true;
    goto clean;
  }

clean:
  for (int i = 0; i < masks.size(); i++) {
    if (masks[i].width == -1) {
      destroy_image(&masks[i]);
    }
  }
  destroy_blender(b);
  return result;
}

bool LendoMerge::merge_six_by_image_path(std::vector<std::string> imgs_path,
                                         std::string out_filename) {

  std::vector<Image> imgs_s;

  for (std::string img_path : imgs_path) {
    imgs_s.push_back(create_image(img_path.c_str()));
  }

  std::vector<Image *> imgs(imgs_s.size());
  for (int i = 0; i < imgs_s.size(); i++) {
    imgs[i] = &imgs_s[i];
  }

  bool result = merge_six(imgs, out_filename.c_str());

  for (Image *img : imgs) {
    destroy_image(img);
  }

  return result;
}
