
#include "LendoMerge.hpp"
#include "simde/simde/x86/avx2.h"
#include "simde/simde/x86/sse2.h"
#include <cstring>
#include <iostream>

int blur_1d_v_simd(int x, int width, int *horizontal_sums[],
                   unsigned char *out_row, int blur_strength) {

  int pp = 0;
  for (; x < width - 8; x += 8) {
    simde__m256i sum = simde_mm256_setzero_si256();
    for (int i = 0; i < blur_strength; i++) {
      simde__m256i r0 = simde_mm256_loadu_si256(
          (const simde__m256i *)(horizontal_sums[i] + x));
      sum = simde_mm256_add_epi32(sum, r0);
    }

    simde__m256 fsum = simde_mm256_cvtepi32_ps(sum);
    const simde__m256 inv25 =
        simde_mm256_set1_ps(1.0f / (blur_strength * blur_strength));
    simde__m256 fres = simde_mm256_mul_ps(fsum, inv25);
    simde__m256i out32 = simde_mm256_cvtps_epi32(fres);

    simde__m128i lo16 = simde_mm256_castsi256_si128(out32);
    simde__m128i hi16 = simde_mm256_extracti128_si256(out32, 1);
    simde__m128i packed = simde_mm_packs_epi32(lo16, hi16);
    simde__m128i out8 = simde_mm_packus_epi16(packed, packed);
    simde_mm_storel_epi64((simde__m128i *)out_row, out8);

    out_row += 8;
  }

  return x;
}

int blur_1d_3c(int x, int width, unsigned char *cur_src, int src_width,
               int *temp_out, int blur_strength) {
  for (; x < width; ++x) {
    for (int c = 0; c < RGB_CHANNELS; c++) {
      int sum = 0;
      for (int i = -blur_strength / 2; i < (blur_strength / 2) + 1; i++) {
        int p = reflect_index(x + i, src_width);
        sum += cur_src[p * RGB_CHANNELS + c];
      }

      temp_out[0] = sum;
      ++temp_out;
    }
  }
  return x;
}

void LendoMerge::blur_image_helper(Image *img, int start, int end,
                                   int blur_strength) {
  blur_strength -= 1;
  int p_blur_strength = max(0, end - start);
  blur_strength = min(blur_strength, p_blur_strength);
  if (blur_strength <= 0) {
    return;
  }
  blur_strength = blur_strength + ((blur_strength % 2) == 0 ? 1 : 0);

  int y = start;
  int j = 0;
  unsigned char *rows[blur_strength];

  for (int i = blur_strength / 2; i >= 0; i--) {
    rows[j] = img->data +
              (reflect_index(-i, img->height)) * img->width * RGB_CHANNELS;
    j++;
  }

  for (int i = 1; i <= blur_strength / 2; i++) {
    rows[j] =
        img->data + (reflect_index(i, img->height)) * img->width * RGB_CHANNELS;
    j++;
  }

  int *temp_dst_out =
      (int *)malloc(blur_strength * img->width * RGB_CHANNELS * sizeof(int));
  if (!temp_dst_out)
    return;

  int *temp_dst_rows[blur_strength];

  for (int i = 0; i < blur_strength; i++) {
    temp_dst_rows[i] = temp_dst_out + (i * img->width * RGB_CHANNELS);
  }

  int s_y = -blur_strength / 2;
  int e_y = blur_strength / 2 + 1;
  for (; y < end; y++) {

    for (; s_y < e_y; s_y++) {
      int x = 0;

      unsigned char *cur_src = rows[s_y + (blur_strength / 2)];
      int *temp_out = temp_dst_rows[s_y + (blur_strength / 2)];
      x = blur_1d_3c(x, min(blur_strength / 2, img->width), cur_src, img->width,
                     temp_out, blur_strength);

      temp_out = temp_out + (x * RGB_CHANNELS);

      for (; x <= img->width - (blur_strength + 1); x += 5) {
        int xx = max(0, x - (blur_strength / 2));

        simde__m256i sum = simde_mm256_setzero_si256();
        for (int i = 0; i < blur_strength; i++) {
          int next_index = xx + i;
          const unsigned char *cur_next_src =
              cur_src + (RGB_CHANNELS * next_index);
          simde__m256i a = simde_mm256_cvtepu8_epi16(simde_mm_loadu_si128((const simde__m128i *)cur_next_src));

          sum = simde_mm256_add_epi16(sum, a);
        }

        simde__m256i lo_sum =
            simde_mm256_cvtepi16_epi32(simde_mm256_castsi256_si128(sum));
        simde__m256i hi_sum =
            simde_mm256_cvtepi16_epi32(simde_mm256_extracti128_si256(sum, 1));

        simde_mm256_storeu_si256((simde__m256i *)temp_out, lo_sum);
        simde_mm256_storeu_si256((simde__m256i *)(temp_out + 8), hi_sum);

        temp_out += (5 * RGB_CHANNELS);
      }

      x = blur_1d_3c(x, img->width, cur_src, img->width, temp_out,
                     blur_strength);
    }

    unsigned char *out_row = img->data + (RGB_CHANNELS * img->width * y);

    int x_vertical = blur_1d_v_simd(0, img->width * RGB_CHANNELS, temp_dst_rows,
                                    out_row, blur_strength) /
                     RGB_CHANNELS;

    out_row = out_row + (x_vertical * RGB_CHANNELS);

    for (; x_vertical < img->width; ++x_vertical) {
      int xx = x_vertical * RGB_CHANNELS;
      for (int c = 0; c < RGB_CHANNELS; c++) {
        int sum = 0;
        for (int i = 0; i < blur_strength; i++) {
          sum += temp_dst_rows[i][xx + c];
        }
        out_row[0] = clamp(sum / (blur_strength * blur_strength), 0, 255);
        ++out_row;
      }
    }

    for (int i = 0; i < blur_strength / 2; i++) {
      rows[i] = rows[blur_strength / 2 + i];
    }

    int add = 1;
    for (int i = blur_strength / 2; i < blur_strength; i++) {
      rows[i] = img->data + (reflect_index(y + add, img->height)) *
                                (img->width * RGB_CHANNELS);
      add++;
    }

    int *temp_store[blur_strength / 2 + 1];
    for (int i = 0; i < blur_strength / 2 + 1; i++) {
      temp_store[i] = temp_dst_rows[i];
    }
    for (int i = 0; i < blur_strength / 2 + 1; i++) {
      temp_dst_rows[i] = temp_dst_rows[blur_strength / 2 + i];
    }
    for (int i = blur_strength / 2 + 1; i < blur_strength; i++) {
      temp_dst_rows[i] = temp_store[i - (blur_strength / 2) - 1];
    }

    s_y = 1;
  }

  free(temp_dst_out);
}

void LendoMerge::blur_image(Image *img, int start, int end) {
  blur_image_helper(img, start, end, blur_strength);
}
