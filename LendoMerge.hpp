#include <memory>
#include <string>
#define GAMMA 2.2
#define K 0.5f
#define BRIGHTNESS 1.5
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x)*M_PI / 180.0)

#include "blending.h"
#include "jpeg.h"
#include <vector>

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
  std::unique_ptr<ImageF> map_x = nullptr, map_y = nullptr;

  void linearize(Image *img, ImageF *out);
  void gamma_encode(ImageF *img, Image *out);
  std::vector<double> compute_alpha(ImageF *prev_img, ImageF *curr_img);
  std::vector<double>
  compute_global_adjustment(std::vector<std::vector<double>> alphas);
  void compute_map(int width, int height, int channels);
  bool merge_top_bottom(Image *img1, Image *img2, const char *merged_filename);
  void color_correct_sequence(const std::vector<Image *> &imgs);
  bool findSeam(Image *img1, Image *img2, Image *mask1, Image *mask2);
  void bilinear_interpolate(Image *img);
  bool merge_images_horizontal(std::vector<Image *> imgs,
                               const char *merged_filename,
                               bool add_height = true);
  bool add_height_to(Image *img);
  bool crop_panorama(Image *img);

public:
  LendoMerge(float hfov, float camera_rotation);
  ~LendoMerge();

  void downsample_image(std::string image_path, std::string out_image_path,
                        int times = 2);
  bool merge_image_path_horizontal(std::vector<std::string> imgs_path,
                                   std::string out_filename,
                                   bool add_height = true);
  bool merge_top_bottom_image_path(std::string image_path_1,
                                   std::string image_path_2,
                                   std::string out_filename);

  bool crop_panorama_by_path(std::string image_path, std::string out_filename);
  bool add_height_to_image_path(std::string image_path,
                                std::string out_filename);
};
