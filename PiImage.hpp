#pragma once

#include "header-files/stitcher/image_operations.hpp"
#include "header-files/stitcher/jpeg.h"
#include <stdexcept>
#include <string>
#include <type_traits>

template <typename PixelT> class PiImage {
public:
  using CType = std::conditional_t<
      std::is_same_v<PixelT, unsigned char>, Image,
      std::conditional_t<std::is_same_v<PixelT, float>, ImageF, ImageS>>;

  PiImage() : img_{nullptr, 0, 0, 0} {}

  PiImage(int w, int h, int c) {
    if constexpr (std::is_same_v<PixelT, unsigned char>) {
      img_ = create_empty_image(w, h, c);
    } else if constexpr (std::is_same_v<PixelT, float>) {
      img_ = create_empty_image_f(w, h, c);
    } else if constexpr (std::is_same_v<PixelT, short>) {
      img_ = create_empty_image_s(w, h, c);
    }

    if (!img_.data)
      throw std::runtime_error("Failed to allocate PiImage data");
  }

  explicit PiImage(const std::string &path) {
    if constexpr (std::is_same_v<PixelT, unsigned char>) {
      img_ = create_image(path.c_str());
      if (!img_.data)
        throw std::runtime_error("Failed to load image from: " + path);
    } else {
      img_ = {nullptr, 0, 0, 0};
      throw std::logic_error(
          "File loading is only supported for 8-bit images (unsigned char)");
    }
  }

  explicit PiImage(CType raw) : img_(raw) {}

  ~PiImage() { reset(); }

  PiImage(const PiImage &) = delete;
  PiImage &operator=(const PiImage &) = delete;

  PiImage(PiImage &&other) noexcept : img_(other.img_) {
    other.img_ = {nullptr, 0, 0, 0};
  }

  PiImage &operator=(PiImage &&other) noexcept {
    if (this != &other) {
      reset();
      img_ = other.img_;
      other.img_ = {nullptr, 0, 0, 0};
    }
    return *this;
  }

  // Basic Accessors
  int width() const { return img_.width; }
  int height() const { return img_.height; }
  int channels() const { return img_.channels; }
  PixelT *data() { return (PixelT *)img_.data; }
  const PixelT *data() const { return (const PixelT *)img_.data; }

  CType *raw() { return &img_; }
  const CType *raw() const { return &img_; }

  CType detach() {
    CType tmp = img_;
    img_ = {nullptr, 0, 0, 0};
    return tmp;
  }

  void reset() {
    if (_clean && img_.data) {
      if constexpr (std::is_same_v<PixelT, unsigned char>) {
        destroy_image(&img_);
      } else if constexpr (std::is_same_v<PixelT, float>) {
        destroy_image_f(&img_);
      } else if constexpr (std::is_same_v<PixelT, short>) {
        destroy_image_s(&img_);
      }
      img_.data = nullptr;
    }
  }

  bool save(std::string out_filename) {
    return save_image(raw(), out_filename.c_str()) > 0;
  }

  bool empty() const { return img_.data == nullptr; }
  void clean(bool clean) { _clean = clean; }

private:
  CType img_;
  bool _clean = true;
};

using PiImageU8 = PiImage<unsigned char>;
using PiImageF = PiImage<float>;
using PiImageS = PiImage<short>;
