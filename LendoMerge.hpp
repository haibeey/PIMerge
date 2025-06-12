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
  bool merge_images(std::vector<Image *> imgs, const char *merged_filename);
  void add_height(Image *img);

public:
  LendoMerge(float hfov, float camera_rotation);
  ~LendoMerge();

  std::string merge(std::vector<std::string>, int img_width,
                    std::string result);
  void downsample_image(std::string image_path, std::string out_image_path);


  bool merge_by_image_path(std::vector<std::string> imgs_path,
                           std::string out_filename);

  bool merge_top_bottom_by_image_path(std::string image_path_1,
                                      std::string image_path_2,
                                      std::string out_filename);
};
