
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>
#include <cstring>
#include <iostream>

#include "LendoMerge.h"

LendoMerge::~LendoMerge() {}

void LendoMerge::findSeam(Image *img1, Image *img2, const char *mask1_filename,
                          const char *mask2_filename) {

  assert(img1->width == img2->width && img1->height == img2->height);
  Image mask1 = convert_RGB_to_gray(img1);
  Image mask2 = convert_RGB_to_gray(img2);

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

    std::memset(mask1.data + (i * img1->width), 255, start);
    std::memset(mask1.data + s, 255, m - s);
    std::memset(mask1.data + m, 0, e - m);

    s = (i * img1->width);
    m = (i * img1->width) + a;

    std::memset(mask2.data + s, 0, m - s);
    std::memset(mask2.data + m, 255, e - m);
  }

  compress_grayscale_jpeg(mask1_filename, &mask1, 50);
  compress_grayscale_jpeg(mask2_filename, &mask2, 50);

  destroy_image(&mask1);
  destroy_image(&mask2);
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

std::vector<double> LendoMerge::compute_alpha(ImageF *prev_img, ImageF *curr_img) {

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


void LendoMerge::bilinear_interpolate(Image *img){

}
