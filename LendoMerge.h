#include <memory>
#include <string>
#define CAMROT 60
#define IMAGE_CUT 0.2739361702
#define GAMMA 2.2
#define K 0.5f

#include "blending.h"
#include "jpeg.h"
#include <vector>

class LendoMerge {
private:
  float FOV, Q, S, beta;
  std::unique_ptr<ImageF> map_x = nullptr, map_y = nullptr;
  int gap = 0;

  void linearize(Image *img, ImageF *out);
  void gamma_encode(ImageF *img, Image *out);
  std::vector<double> compute_alpha(ImageF *prev_img, ImageF *curr_img);
  std::vector<double>
  compute_global_adjustment(std::vector<std::vector<double>> alphas);
  void compute_map(int width, int height, int channels);
  bool merge_two(Image *img1, Image *img2, const char *merged_filename);
  void color_correct_sequence(const std::vector<Image *> &imgs);
  bool findSeam(Image *img1, Image *img2, Image *mask1, Image *mask2);
  void bilinear_interpolate(Image *img);

public:
  LendoMerge(float FOV) : FOV(FOV) {
      beta = CAMROT - (FOV / 2);

  };
  ~LendoMerge();
  std::string merge(std::vector<std::string>, int img_width,std::string result);
  void downsample_image(std::string image_path,std::string out_image_path);
  bool merge_two_by_image_path(std::string image_path_1,
                               std::string image_path_2,
                               std::string out_filename);
};
