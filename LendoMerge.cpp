
#include "simde/simde/x86/avx2.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <malloc/_malloc.h>
#include <memory>
#include <vector>

#include "LendoMerge.h"

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

  auto min_element =
      std::min_element(dp[err_width - 1].begin(), dp[err_width - 1].end());
  int index = std::distance(dp[err_width - 1].begin(), min_element);

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

    std::memset(mask1->data + (i * img1->width), 255, start);
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

  for (int i = 1; i < imgs.size(); i++) {
    alphas.push_back(
        compute_alpha(&linearize_images[i - 1], &linearize_images[i]));
  }

  std::vector<double> g = compute_global_adjustment(alphas);

  for (int i = 0; i < imgs.size(); i++) {
    std::vector<double> combined;
    for (int j = 0; j < 3; j++) {
      combined.push_back(std::pow((g[j] * alphas[i][j]), (1.0 / GAMMA)));
    }

    for (int k = 0; k < (imgs[0]->width * imgs[0]->height * imgs[0]->channels);
         k++) {
      linearize_images[i].data[k] =
          linearize_images[i].data[k] * combined[k % 3];
    }

    gamma_encode(&linearize_images[i], imgs[i]);
  }

  for (ImageF imgf : linearize_images) {
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
  Rect out_size = {
      0, 0, (img1->width * 2) - static_cast<int>((img1->width * IMAGE_CUT)),
      img1->height};

  printf("%d %d \n", mask1.width, mask1.height);

  Blender *b = create_blender(MULTIBAND, out_size, 5);

  feed(b, img1, &mask1, Point{0, 0});
  feed(b, img2, &mask2,
       Point{img1->width - ( 2 * static_cast<int>(img1->width * IMAGE_CUT)), 0});
  blend(b);

  destroy_image(&mask1);
  destroy_image(&mask2);

  if (b->result.data != NULL) {
    bilinear_interpolate(&b->result);
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
