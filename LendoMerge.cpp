
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

int mod(int a, int b) { return (a % b + b) % b; }
float distanceBetween(const MergePoint &p1, const MergePoint &p2) {
  float dx = p2.x - p1.x;
  float dy = p2.y - p1.y;
  return std::sqrt(dx * dx + dy * dy);
}

MergePoint getIntersection(const MergeLine &line1, const MergeLine &line2) {
  float x1 = line1.A.x, y1 = line1.A.y;
  float x2 = line1.B.x, y2 = line1.B.y;
  float x3 = line2.A.x, y3 = line2.A.y;
  float x4 = line2.B.x, y4 = line2.B.y;

  float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
  assert(denom != 0.0f);
  float px =
      ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) /
      denom;

  float py =
      ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) /
      denom;

  return MergePoint{px, py};
}

LendoMerge::LendoMerge(float hfov, float camera_rotation, int bs) {
  blur_strength = bs;
  float half_angle_deg = hfov / 2.0;

  float half_angle_rad = DEG2RAD(half_angle_deg);
  float reference_angle_rad_1 = DEG2RAD(-half_angle_deg + camera_rotation);
  float reference_angle_rad_2 = DEG2RAD(half_angle_deg + camera_rotation);

  float unit_lenght = 2000.0;

  MergeLine main_line_1 = {
      {0.0f, 0.0f},
      {-unit_lenght * sin(half_angle_rad), unit_lenght * cos(half_angle_rad)}};

  MergeLine main_line_2 = {
      {0.0f, 0.0f},
      {unit_lenght * sin(half_angle_rad), unit_lenght * cos(half_angle_rad)}};

  MergeLine connect_line = {main_line_1.B, main_line_2.B};

  MergeLine ref_line_1 = {{0.0f, 0.0f},
                          {unit_lenght * sin(reference_angle_rad_1),
                           unit_lenght * cos(reference_angle_rad_1)}};

  MergeLine ref_line_2 = {{0.0f, 0.0f},
                          {unit_lenght * sin(reference_angle_rad_2),
                           unit_lenght * cos(reference_angle_rad_2)}};

  MergeLine connect_ref = {ref_line_1.B, ref_line_2.B};

  MergePoint intersection = getIntersection(connect_line, connect_ref);

  float dist_a = distanceBetween(connect_line.A, connect_line.B);
  float dist_b = distanceBetween(intersection, connect_line.B);
  image_cut = dist_b / dist_a;
}

LendoMerge::~LendoMerge() {
  if (map_x != nullptr)
    destroy_image_f(map_x.get());
  if (map_y != nullptr)
    destroy_image_f(map_y.get());
}

bool LendoMerge::findSeam(Image *img1, Image *img2, Image *mask1, Image *mask2,
                          bool is_new_img1, bool is_new_img2) {

  assert(img1->width == img2->width && img1->height == img2->height);

  int err_width = static_cast<int>(img1->width * image_cut);
  int start = static_cast<int>(img1->width * (1 - image_cut));

  std::vector<std::vector<short>> E(img1->height,
                                    std::vector<short>(err_width));

  for (int i = 0; i < img1->height; i++) {
    for (int j = 0; j < err_width; j++) {
      unsigned char a = mask1->data[(i * mask1->width) + start + j];
      unsigned char b = mask2->data[(i * mask2->width) + j];
      E[i][j] = (b - a) * (b - a);
    }
  }

  if (is_new_img1) {
    std::memset(mask1->data, 255,
                mask1->channels * mask1->width * mask1->height);
  }

  if (is_new_img2) {
    std::memset(mask2->data, 255,
                mask2->channels * mask2->width * mask2->height);
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
  int overlap_width = static_cast<int>(prev_img->width * image_cut);
  int start_overlap_width = static_cast<int>(prev_img->width * (1 - image_cut));

  std::vector<double> sums_prev(3, 0.0);
  std::vector<double> sums_cur(3, 0.0);

  for (int channel = 0; channel < prev_img->channels; channel++) {
    for (int i = 0; i < prev_img->height; i++) {
      for (int j = 0; j < overlap_width; j++) {
        int cur_j = j;
        int prev_j = j + start_overlap_width;

        if (cur_j >= prev_img->width || prev_j >= prev_img->width) {
          continue;
        }

        int cur_pos =
            ((i * prev_img->width) + cur_j) * prev_img->channels + channel;
        int prev_pos =
            ((i * prev_img->width) + prev_j) * prev_img->channels + channel;

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

  linearize_images.clear();
}

void LendoMerge::downsample_image(std::string image_path,
                                  std::string out_image_path, int times) {
  Image img = create_image(image_path.c_str());
  if (img.width <= 0 || img.height <= 0) {
    return;
  }
  Image down;
  while (img.width > 400 && times > 0) {
    down = downsample(&img);
    destroy_image(&img);
    img = down;
    times--;
  }
  save_image(&img, out_image_path.c_str());
  destroy_image(&img);
}

bool LendoMerge::add_height_to(Image *img) {
  if (img->width <= 0 || img->height <= 0)
    return false;
  int new_width = img->width;
  int new_height = img->height;
  to_add = 0;
  if (img->height < new_width / 2) {
    to_add = (new_width / 2) - new_height;
    new_height += to_add;
  }

  Image new_img = create_empty_image(new_width, new_height, img->channels);
  int half_to_add = to_add / 2;

  for (int y = 0; y < half_to_add + 1; y++) {
    unsigned char *image_start =
        img->data +
        (((half_to_add - y) % img->height) * img->width * new_img.channels);
    unsigned char *new_image_start =
        new_img.data + ((y % new_height) * img->width * new_img.channels);
    memcpy(new_image_start, image_start, new_img.width * new_img.channels);
  }

  int yy = 0;
  for (int y = (half_to_add);
       y < new_height - (half_to_add) && yy < img->height; y++) {

    unsigned char *new_image_start =
        new_img.data + (y * new_img.width * new_img.channels);
    unsigned char *image_start =
        img->data + (yy * img->width * new_img.channels);
    memcpy(new_image_start, image_start, new_img.width * new_img.channels);
    yy++;
  }

  for (int y = 0; y < half_to_add; y++) {
    unsigned char *image_start =
        img->data +
        (mod(img->height - y - 1, img->height) * img->width * new_img.channels);
    unsigned char *new_image_start =
        new_img.data + (mod(img->height + half_to_add + y, new_height) *
                        img->width * new_img.channels);
    memcpy(new_image_start, image_start, new_img.width * new_img.channels);
  }

  blur_image(&new_img, 0, half_to_add);
  blur_image(&new_img, img->height + half_to_add, new_img.height);

  free(img->data);
  img->data = new_img.data;
  img->width = new_width;
  img->height = new_height;

  return true;
}

bool LendoMerge::merge_images_horizontal(std::vector<Image *> imgs,
                                         const char *merged_filename,
                                         bool add_height) {

  assert(imgs.size() > 0);
  for (int i = 0; i < imgs.size(); i++) {
    if (imgs[i]->width <= 0 || imgs[i]->height <= 0) {
      return false;
    }
  }
  color_correct_sequence(imgs);
  bool result = false;
  int bands = 5;
  int out_height = 0;
  int out_width =
      static_cast<int>(imgs[0]->width * imgs.size()) -
      (static_cast<int>((imgs.size() - 1)) * static_cast<int>(imgs[0]->width * image_cut));
  std::vector<Image> masks(imgs.size());
  std::vector<int> x_points(imgs.size());
  for (int i = 0; i < imgs.size(); i++) {
    masks[i].width = -1;
  }
  x_points[0] = 0;
  Blender *b = nullptr;
  StitchRect out_size;

  int gap = (1 << bands);

  for (int i = 0; i < imgs.size(); i++) {
    masks[i] = convert_RGB_to_gray(imgs[i]);
  }

  for (int i = 0; i < imgs.size() - 1; i++) {
    bool new_img_1 = false;
    bool new_img_2 = false;

    if (i == 0) {
      new_img_1 = true;
      new_img_2 = true;
    } else {
      new_img_2 = true;
    }

    if (!findSeam(imgs[i], imgs[i + 1], &masks[i], &masks[i + 1], new_img_1,
                  new_img_2))
      goto clean;
  }

  out_size = {0, 0, out_width, imgs[0]->height};
  b = create_blender(MULTIBAND, out_size, bands);
  out_height = b->output_size.height;

  out_width = 0;

  for (int i = 0; i < imgs.size(); i++) {
    if (i < imgs.size() - 1) {
      out_width +=
          (masks[i].width * 2) - static_cast<int>(masks[i].width * image_cut);
      x_points[i + 1] = x_points[i] + masks[i].width -
                        static_cast<int>(masks[i].width * image_cut) - gap;
      b->output_size = {0, 0, out_width, out_height};
    }

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
    int join = static_cast<int>(imgs[0]->width * image_cut) + right_cut;
    crop_image(&b->result, 0, 0, 0, join);
    if (add_height)
      add_height_to(&b->result);

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

bool LendoMerge::merge_image_path_horizontal(std::vector<std::string> imgs_path,
                                             std::string out_filename,
                                             bool add_height) {

  std::vector<Image> imgs_s;

  for (std::string img_path : imgs_path) {
    imgs_s.push_back(create_image(img_path.c_str()));
  }

  std::vector<Image *> imgs(imgs_s.size());
  for (int i = 0; i < imgs_s.size(); i++) {
    imgs[i] = &imgs_s[i];
  }

  bool result = merge_images_horizontal(imgs, out_filename.c_str(), add_height);

  for (Image *img : imgs) {
    destroy_image(img);
  }

  return result;
}

bool LendoMerge::merge_top_bottom(Image *img1, Image *img2,
                                  const char *merged_filename) {

  if (img1->width <= 0 || img1->height <= 0 || img2->width <= 0 ||
      img2->height <= 0) {
    return false;
  }

  Image mask1 = create_vertical_mask(img1->width, img1->height, 0.5, 0, 1);
  Image mask2 = create_empty_image(img2->width, img2->height, 1);

  memset(mask2.data, 255, mask2.width * mask2.height);

  int halfH = img1->height / 2;
  StitchRect out_size = {
      .x = 0, .y = 0, .width = img1->width, .height = halfH + img2->height};

  const int bands = 2;
  Blender *b = create_blender(FEATHER, out_size, bands);
  if (!b) {
    destroy_image(&mask1);
    destroy_image(&mask2);
    return false;
  }

  feed(b, img1, &mask1, (StitchPoint){0, 0});
  feed(b, img2, &mask2, (StitchPoint){0, halfH});

  blend(b);
  add_height_to(&b->result);

  bool saved = save_image(&b->result, merged_filename);

  destroy_image(&mask1);
  destroy_image(&mask2);
  destroy_blender(b);

  return saved;
}

bool LendoMerge::merge_top_bottom_image_path(std::string image_path_1,
                                             std::string image_path_2,
                                             std::string out_filename) {

  Image img1 = create_image(image_path_1.c_str());
  Image img2 = create_image(image_path_2.c_str());

  bool result = merge_top_bottom(&img1, &img2, out_filename.c_str());

  destroy_image(&img1);
  destroy_image(&img2);

  return result;
}

bool LendoMerge::crop_panorama(Image *img) {

  if (img->width <= 0 || img->height <= 0) {
    return false;
  }
  int fifth_of_height = static_cast<int>(0.3 * img->height);

  int y = 20;
  int stride = (img->width * img->channels) - img->channels;
  for (; y < fifth_of_height; y++) {
    int pos = (y * img->width) * img->channels;
    bool same = true;
    for (int c = 0; c < img->channels; c++) {
      if (img->data[c] != img->data[pos + c] ||
          img->data[stride + c] != img->data[pos + stride + c]) {
        same = false;
        break;
      }
    }
    if (same) {
      break;
    }
  }

  crop_image(img, y, y, 0, 0);

  return true;
}

bool LendoMerge::crop_panorama_by_path(std::string image_path,
                                       std::string out_filename) {
  Image img = create_image(image_path.c_str());
  bool result = crop_panorama(&img);
  if (!save_image(&img, out_filename.c_str())) {
    result = false;
  }
  destroy_image(&img);
  return result;
}

bool LendoMerge::add_height_to_image_path(std::string image_path,
                                          std::string out_filename) {
  Image img = create_image(image_path.c_str());

  bool result = add_height_to(&img);

  if (!save_image(&img, out_filename.c_str())) {
    result = false;
  }

  destroy_image(&img);
  return result;
}
