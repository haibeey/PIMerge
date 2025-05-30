#include <memory>
#define CAMROT 60
#define IMAGE_CUT 0.2739361702
#define GAMMA 2.2
#define K 0.8f

#include "blending.h"
#include "jpeg.h"
#include <vector>

class LendoMerge {
private:
  float FOV, Q, S, beta;
  std::unique_ptr<ImageF> map_x = nullptr, map_y = nullptr;

  void linearize(Image *img, ImageF *out);
  void gamma_encode(ImageF *img, Image *out);
  std::vector<double> compute_alpha(ImageF *prev_img, ImageF *curr_img);
  std::vector<double>
  compute_global_adjustment(std::vector<std::vector<double>> alphas);
  void compute_map(int width, int height, int channels);

public:
  LendoMerge(float FOV) : FOV(FOV) { beta = CAMROT - (FOV / 2); };
  ~LendoMerge();
  void findSeam(Image *img1, Image *img2, const char *mask1_filename,
                const char *mask2_filename);
  void color_correct_sequence(const std::vector<Image *> &imgs);
  void bilinear_interpolate(Image *img);
};
