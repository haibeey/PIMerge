

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "PIMerge.hpp"
#include "PiImage.hpp"

int mod(int a, int b) { return (a % b + b) % b; }
float distanceBetween(const MergePoint &p1, const MergePoint &p2) {
  float dx = p2.x - p1.x;
  float dy = p2.y - p1.y;
  return std::sqrt(dx * dx + dy * dy);
}

// Compute Sobel gradient magnitude at pixel (y, x) across all channels.
// High values = strong edges (bad seam locations). Low values = smooth regions
// (good seam locations).
static float gradient_magnitude(PiImageU8 &img, int y, int x) {
  const int w = img.width(), h = img.height(), c = img.channels();
  const int x0 = std::max(0, x - 1), x2 = std::min(w - 1, x + 1);
  const int y0 = std::max(0, y - 1), y2 = std::min(h - 1, y + 1);
  float gx_sq = 0.0f, gy_sq = 0.0f;
  for (int ch = 0; ch < c; ch++) {
    float tl = img.data()[(y0 * w + x0) * c + ch];
    float tc = img.data()[(y0 * w + x) * c + ch];
    float tr = img.data()[(y0 * w + x2) * c + ch];
    float ml = img.data()[(y * w + x0) * c + ch];
    float mr = img.data()[(y * w + x2) * c + ch];
    float bl = img.data()[(y2 * w + x0) * c + ch];
    float bc = img.data()[(y2 * w + x) * c + ch];
    float br = img.data()[(y2 * w + x2) * c + ch];
    float gxc = -tl + tr - 2.0f * ml + 2.0f * mr - bl + br;
    float gyc = tl + 2.0f * tc + tr - bl - 2.0f * bc - br;
    gx_sq += gxc * gxc;
    gy_sq += gyc * gyc;
  }
  return std::sqrt(gx_sq + gy_sq);
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

LendoMerge::~LendoMerge() {}

bool LendoMerge::findSeam(PiImageU8 &img1, PiImageU8 &img2, PiImageU8 &mask1,
                          PiImageU8 &mask2, bool is_new_img1,
                          bool is_new_img2) {

  assert(img1.width() == img2.width() && img1.height() == img2.height());

  const int err_width = static_cast<int>(img1.width() * image_cut);
  const int start = static_cast<int>(img1.width() * (1 - image_cut));
  const float INF = std::numeric_limits<float>::max();

  // Energy function: RGB color difference + Sobel gradient magnitude.
  // Seam prefers regions where both images look similar (low color diff)
  // AND have low texture (low gradient) — avoids cutting through visible edges.
  std::vector<std::vector<float>> E(img1.height(),
                                    std::vector<float>(err_width));

  for (int i = 0; i < img1.height(); i++) {
    for (int j = 0; j < err_width; j++) {
      // Per-channel squared color difference in the overlap region
      float color_diff = 0.0f;
      for (int ch = 0; ch < img1.channels(); ch++) {
        float a =
            img1.data()[((i * img1.width()) + start + j) * img1.channels() +
                        ch];
        float b = img2.data()[((i * img2.width()) + j) * img2.channels() + ch];
        float d = b - a;
        color_diff += d * d;
      }
      // Gradient penalty: discourage seams through edges in either image
      float grad1 = gradient_magnitude(img1, i, start + j);
      float grad2 = gradient_magnitude(img2, i, j);
      E[i][j] = color_diff + (grad1 + grad2);
    }
  }

  if (is_new_img1) {
    std::memset(mask1.data(), 255,
                mask1.channels() * mask1.width() * mask1.height());
  }

  if (is_new_img2) {
    std::memset(mask2.data(), 255,
                mask2.channels() * mask2.width() * mask2.height());
  }

  // Dynamic programming: find minimum-cost vertical seam through the overlap
  std::vector<std::vector<float>> dp(img1.height(),
                                     std::vector<float>(err_width));
  for (int i = 0; i < err_width; i++) {
    dp[0][i] = E[0][i];
  }

  for (int i = 1; i < img1.height(); i++) {
    for (int j = 0; j < err_width; j++) {
      float a = (j > 0) ? dp[i - 1][j - 1] : INF;
      float b = dp[i - 1][j];
      float c = (j < err_width - 1) ? dp[i - 1][j + 1] : INF;
      dp[i][j] = E[i][j] + std::min({a, b, c});
    }
  }

  E.clear();

  auto min_it = std::min_element(dp[img1.height() - 1].begin(),
                                 dp[img1.height() - 1].end());
  int index =
      static_cast<int>(std::distance(dp[img1.height() - 1].begin(), min_it));

  std::vector<int> path;
  path.reserve(img1.height());
  path.push_back(index);

  for (int i = img1.height() - 2; i >= 0; i--) {
    float a = (index > 0) ? dp[i][index - 1] : INF;
    float b = dp[i][index];
    float c = (index < err_width - 1) ? dp[i][index + 1] : INF;

    if (a <= b && a <= c) {
      index--;
    } else if (c < a && c < b) {
      index++;
    }
    path.push_back(index);
  }

  dp.clear();

  std::reverse(path.begin(), path.end());

  for (int i = 0; i < img1.height(); i++) {
    const int a = path[i];
    const int row_base = i * img1.width();

    std::memset(mask1.data() + row_base + start, 255, a);
    std::memset(mask1.data() + row_base + start + a, 0,
                img1.width() - start - a);

    std::memset(mask2.data() + row_base, 0, a);
    std::memset(mask2.data() + row_base + a, 255, img2.width() - a);
  }

  return true;
}

void LendoMerge::linearize(PiImageU8 &img, PiImageF &out) {
  assert(img.width() == out.width() && img.height() == out.height() &&
         img.channels() == out.channels());
  for (int i = 0; i < (img.width() * img.height() * out.channels()); i++) {
    out.data()[i] = std::pow((img.data()[i] / 255.0), GAMMA);
  }
}

void LendoMerge::gamma_encode(PiImageF &img, PiImageU8 &out) {
  assert(img.width() == out.width() && img.height() == out.height() &&
         img.channels() == out.channels());

  float gamma_inv = 1.0 / GAMMA;
  for (int i = 0; i < (img.width() * img.height() * out.channels()); i++) {
    float v = std::pow(img.data()[i], gamma_inv) * 255.0;
    unsigned char p = static_cast<unsigned char>(v);
    if (v < 0) {
      p = 0;
    } else if (v > 255) {
      p = 255;
    }
    out.data()[i] = p;
  }
}

std::vector<double> LendoMerge::compute_alpha(PiImageF &prev_img,
                                              PiImageF &curr_img) {
  int overlap_width = static_cast<int>(prev_img.width() * image_cut);
  int start_overlap_width =
      static_cast<int>(prev_img.width() * (1 - image_cut));

  std::vector<double> sums_prev(RGB_CHANNELS, 0.0);
  std::vector<double> sums_cur(RGB_CHANNELS, 0.0);

  for (int channel = 0; channel < prev_img.channels(); channel++) {
    for (int i = 0; i < prev_img.height(); i++) {
      for (int j = 0; j < overlap_width; j++) {
        int cur_j = j;
        int prev_j = j + start_overlap_width;

        if (cur_j >= prev_img.width() || prev_j >= prev_img.width()) {
          continue;
        }

        int cur_pos =
            ((i * prev_img.width()) + cur_j) * prev_img.channels() + channel;
        int prev_pos =
            ((i * prev_img.width()) + prev_j) * prev_img.channels() + channel;

        sums_prev[channel] += prev_img.data()[prev_pos];
        sums_cur[channel] += curr_img.data()[cur_pos];
      }
    }
  }

  std::vector<double> result;
  for (int i = 0; i < sums_prev.size(); i++) {
    result.push_back(sums_prev[i] / (sums_cur[i] + 1e-8));
  }

  return result;
}

std::vector<double> LendoMerge::compute_global_adjustment(
    std::vector<std::vector<double>> &alphas) {

  std::vector<double> result = std::vector<double>(RGB_CHANNELS);
  std::vector<double> alphas_sq = std::vector<double>(RGB_CHANNELS);
  for (int i = 0; i < RGB_CHANNELS; i++) {
    for (int j = 0; j < alphas.size(); j++) {
      result[i] += alphas[j][i];
      alphas_sq[i] += std::pow(alphas[j][i], 2.0);
    }
  }

  for (int i = 0; i < RGB_CHANNELS; i++) {
    result[i] = result[i] / alphas_sq[i];
  }

  return result;
}

void LendoMerge::color_correct_sequence(std::vector<PiImageU8> &imgs) {
  std::vector<PiImageF> linearize_images;
  for (int i = 0; i < imgs.size(); i++) {
    linearize_images.push_back(
        PiImageF(imgs[i].width(), imgs[i].height(), imgs[i].channels()));
    linearize(imgs[i], linearize_images[i]);
  }

  std::vector<std::vector<double>> alphas;
  alphas.push_back({1.0, 1.0, 1.0});
  for (int i = 1; i < imgs.size(); i++)
    alphas.push_back(
        compute_alpha(linearize_images[i - 1], linearize_images[i]));

  std::vector<double> g = compute_global_adjustment(alphas);

  for (int i = 0; i < imgs.size(); i++) {
    std::vector<double> combined(3);
    for (int c = 0; c < 3; c++) {
      double gain = g[c] * alphas[i][c];
      combined[c] = std::pow(gain, 1.0 / GAMMA) * BRIGHTNESS;
    }

    const int total = imgs[i].width() * imgs[i].height() * imgs[i].channels();
    for (int k = 0; k < total; k++) {
      linearize_images[i].data()[k] *= combined[k % 3];
    }
    gamma_encode(linearize_images[i], imgs[i]);
  }
}

void LendoMerge::downsample_image(std::string image_path,
                                  std::string out_image_path, int max_width,int times) {
  PiImageU8 img = PiImageU8(image_path);
  if (img.width() <= 0 || img.height() <= 0) {
    return;
  }
  Image down;
  while (img.width() > max_width && times > 0) {
    down = downsample(img.raw());
    img = PiImageU8(down);
    times--;
  }
  img.save(out_image_path);
}

bool LendoMerge::add_height_to(PiImageU8 &img) {
  if (img.width() <= 0 || img.height() <= 0)
    return false;
  int new_width = img.width();
  int new_height = img.height();
  to_add = 0;
  if (img.height() < new_width / 2) {
    to_add = (new_width / 2) - new_height;
    new_height += to_add;
  }

  PiImageU8 new_img(new_width, new_height, img.channels());
  int half_to_add = to_add / 2;

  for (int y = 0; y < half_to_add + 1; y++) {
    unsigned char *image_start =
        img.data() +
        (((half_to_add - y) % img.height()) * img.width() * new_img.channels());
    unsigned char *new_image_start =
        new_img.data() + ((y % new_height) * img.width() * new_img.channels());
    memcpy(new_image_start, image_start, new_img.width() * new_img.channels());
  }

  int yy = 0;
  for (int y = (half_to_add);
       y < new_height - (half_to_add) && yy < img.height(); y++) {

    unsigned char *new_image_start =
        new_img.data() + (y * new_img.width() * new_img.channels());
    unsigned char *image_start =
        img.data() + (yy * img.width() * new_img.channels());
    memcpy(new_image_start, image_start, new_img.width() * new_img.channels());
    yy++;
  }

  for (int y = 0; y < half_to_add; y++) {
    unsigned char *image_start =
        img.data() + (mod(img.height() - y - 1, img.height()) * img.width() *
                      new_img.channels());
    unsigned char *new_image_start =
        new_img.data() + (mod(img.height() + half_to_add + y, new_height) *
                          img.width() * new_img.channels());
    memcpy(new_image_start, image_start, new_img.width() * new_img.channels());
  }

  blur_image(new_img, 0, half_to_add);
  blur_image(new_img, img.height() + half_to_add, new_img.height());

  if (half_to_add > 1) {
    const int W = new_img.width() * new_img.channels();

    const unsigned char *orig_top = new_img.data() + half_to_add * W;
    for (int y = 0; y < half_to_add; y++) {
      float t = (float)y / (half_to_add - 1);  // 0 → 1
      float alpha = t * t * (3.0f - 2.0f * t); // smoothstep
      unsigned char *row = new_img.data() + y * W;
      for (int x = 0; x < W; x++) {
        row[x] = static_cast<unsigned char>(alpha * orig_top[x] +
                                            (1.0f - alpha) * row[x] + 0.5f);
      }
    }

    const int bot_start = half_to_add + img.height();
    const unsigned char *orig_bot = new_img.data() + (bot_start - 1) * W;
    for (int y = bot_start; y < new_height; y++) {
      float t = (float)(new_height - 1 - y) / (half_to_add - 1); // 1 → 0
      float alpha = t * t * (3.0f - 2.0f * t);                   // smoothstep
      unsigned char *row = new_img.data() + y * W;
      for (int x = 0; x < W; x++) {
        row[x] = static_cast<unsigned char>(alpha * orig_bot[x] +
                                            (1.0f - alpha) * row[x] + 0.5f);
      }
    }
  }

  img = std::move(new_img);

  return true;
}

bool LendoMerge::merge_images_horizontal(std::vector<PiImageU8> &imgs,
                                         const std::string merged_filename,
                                         bool add_height) {

  assert(imgs.size() > 0);
  for (int i = 0; i < imgs.size(); i++) {
    if (imgs[i].width() <= 0 || imgs[i].height() <= 0) {
      return false;
    }
  }
  color_correct_sequence(imgs);
  int bands = 5;
  int out_height = 0;
  int out_width = static_cast<int>(imgs[0].width() * imgs.size()) -
                  (static_cast<int>((imgs.size() - 1)) *
                   static_cast<int>(imgs[0].width() * image_cut));
  std::vector<PiImageU8> masks(imgs.size());
  std::vector<int> x_points(imgs.size());

  x_points[0] = 0;

  StitchRect out_size;

  int gap = (1 << bands);

  for (int i = 0; i < imgs.size(); i++) {
    masks[i] = PiImageU8(convert_RGB_to_gray(imgs[i].raw()));
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

    if (!findSeam(imgs[i], imgs[i + 1], masks[i], masks[i + 1], new_img_1,
                  new_img_2))
      return false;
  }

  Blender *b = nullptr;
  out_size = {0, 0, out_width, imgs[0].height()};
  b = create_blender(MULTIBAND, out_size, bands);
  out_height = b->output_size.height;

  out_width = 0;

  for (int i = 0; i < imgs.size(); i++) {
    if (i < imgs.size() - 1) {
      out_width += (masks[i].width() * 2) -
                   static_cast<int>(masks[i].width() * image_cut);
      x_points[i + 1] = x_points[i] + masks[i].width() -
                        static_cast<int>(masks[i].width() * image_cut) - gap;
      b->output_size = {0, 0, out_width, out_height};
    }

    feed(b, imgs[i].raw(), masks[i].raw(), StitchPoint{x_points[i], 0});
  }

  blend(b);

  int right_cut = -1;
  if (b->result.data != NULL) {
    for (int i = (b->result.width * RGB_CHANNELS) - 1; i > 200; i--) {
      if (b->result.data[i] > 0) {
        right_cut = b->result.width - (i / RGB_CHANNELS);
        break;
      }
    }
  }

  bool ok = false;
  PiImageU8 result;

  if (right_cut >= 0) {
    int join = static_cast<int>(imgs[0].width() * image_cut) + right_cut;
    crop_image(&b->result, 0, 0, 0, join);
    result = PiImageU8(b->result);
    result.clean(false);
    if (add_height)
      add_height_to(result);
    ok = result.save(merged_filename);
  }

  destroy_blender(b);
  return ok;
}

bool LendoMerge::merge_image_path_horizontal(
    std::vector<std::string> &imgs_path, std::string out_filename,
    bool add_height) {

  std::vector<PiImageU8> imgs;

  for (std::string img_path : imgs_path) {
    imgs.push_back(PiImageU8(img_path));
  }

  bool result = merge_images_horizontal(imgs, out_filename, add_height);

  return result;
}

bool LendoMerge::merge_top_bottom(PiImageU8 &img1, PiImageU8 &img2,
                                  const std::string merged_filename) {

  if (img1.width() <= 0 || img1.height() <= 0 || img2.width() <= 0 ||
      img2.height() <= 0) {
    return false;
  }

  PiImageU8 mask1(create_vertical_mask(img1.width(), img1.height(), 0.5, 0, 1));
  PiImageU8 mask2(img2.width(), img2.height(), 1);

  memset(mask2.data(), 255, mask2.width() * mask2.height());

  int halfH = img1.height() / 2;
  StitchRect out_size = {
      .x = 0, .y = 0, .width = img1.width(), .height = halfH + img2.height()};

  const int bands = 2;
  Blender *b = create_blender(FEATHER, out_size, bands);
  if (!b) {
    return false;
  }

  feed(b, img1.raw(), mask1.raw(), (StitchPoint){0, 0});
  feed(b, img2.raw(), mask2.raw(), (StitchPoint){0, halfH});

  blend(b);
  PiImageU8 result = PiImageU8(b->result);
  result.clean(false); // don't clean data.
  add_height_to(result);

  bool saved = result.save(merged_filename);

  destroy_blender(b);

  return saved;
}

bool LendoMerge::merge_top_bottom_image_path(std::string image_path_1,
                                             std::string image_path_2,
                                             std::string out_filename) {

  PiImageU8 img1 = PiImageU8(image_path_1.c_str());
  PiImageU8 img2 = PiImageU8(image_path_2.c_str());

  bool result = merge_top_bottom(img1, img2, out_filename);

  return result;
}

bool LendoMerge::crop_panorama(PiImageU8 &img) {

  if (img.width() <= 0 || img.height() <= 0) {
    return false;
  }
  int fifth_of_height = static_cast<int>(0.3 * img.height());

  int y = 20;
  int stride = (img.width() * img.channels()) - img.channels();
  for (; y < fifth_of_height; y++) {
    int pos = (y * img.width()) * img.channels();
    bool same = true;
    for (int c = 0; c < img.channels(); c++) {
      if (img.data()[c] != img.data()[pos + c] ||
          img.data()[stride + c] != img.data()[pos + stride + c]) {
        same = false;
        break;
      }
    }
    if (same) {
      break;
    }
  }

  crop_image(img.raw(), y, y, 0, 0);

  return true;
}

bool LendoMerge::crop_panorama_by_path(std::string image_path,
                                       std::string out_filename) {

  PiImageU8 img(image_path);
  return crop_panorama(img) && img.save(out_filename);
}

bool LendoMerge::add_height_to_image_path(std::string image_path,
                                          std::string out_filename) {
  PiImageU8 img(image_path);
  return add_height_to(img) && img.save(out_filename);
}
