
#include "simde/simde/x86/avx2.h"
#include "LendoMerge.hpp"

int blur_1d_v_simd(int x, int width, int *row0, int *row1, int *row2, int *row3,
                   int *row4, unsigned char *out_row) {

  for (; x < width - 8; x += 8) {
    simde__m256i r0 = simde_mm256_loadu_si256((const simde__m256i *)(row0 + x));
    simde__m256i r1 = simde_mm256_loadu_si256((const simde__m256i *)(row1 + x));
    simde__m256i r2 = simde_mm256_loadu_si256((const simde__m256i *)(row2 + x));
    simde__m256i r3 = simde_mm256_loadu_si256((const simde__m256i *)(row3 + x));
    simde__m256i r4 = simde_mm256_loadu_si256((const simde__m256i *)(row4 + x));

    simde__m256i out = simde_mm256_srai_epi32(
        simde_mm256_add_epi32(
            r0,
            simde_mm256_add_epi32(
                r1, simde_mm256_add_epi32(r2, simde_mm256_add_epi32(r3, r4)))),
        4);

    simde_mm256_storeu_si256((simde__m256i *)out_row, out);

    out_row += 8;
  }

  return x;
}

int blur_1d_3c(int x, int width, unsigned char *cur_src, int src_width,
               int *temp_out) {
  for (; x < width; ++x) {
    for (int c = 0; c < RGB_CHANNELS; c++) {
      int p0 = reflect_index(x - 2, src_width),
          p1 = reflect_index(x - 1, src_width), p2 = x,
          p3 = reflect_index(x + 1, src_width),
          p4 = reflect_index(x + 2, src_width);
      int s0 = cur_src[p0 * RGB_CHANNELS + c];
      int s1 = cur_src[p1 * RGB_CHANNELS + c];
      int s2 = cur_src[p2 * RGB_CHANNELS + c];
      int s3 = cur_src[p3 * RGB_CHANNELS + c];
      int s4 = cur_src[p4 * RGB_CHANNELS + c];

      temp_out[0] = s0 + s1 + s2 + s3 + s4;
      ++temp_out;
    }
  }
  return x;
}

void LendoMerge::blur_image(Image *img, int start, int end) {
  int y = start;

  unsigned char *rows[5] = {img->data + (reflect_index(y - 2, img->height)) *
                                            img->width * RGB_CHANNELS,
                            img->data + (reflect_index(y - 1, img->height)) *
                                            img->width * RGB_CHANNELS,
                            img->data + y * img->width * RGB_CHANNELS,
                            img->data + (reflect_index(y + 1, img->height)) *
                                            img->width * RGB_CHANNELS,
                            img->data + (reflect_index(y + 2, img->height)) *
                                            img->width * RGB_CHANNELS};

  int *temp_dst_out =
      (int *)malloc(5 * img->width * RGB_CHANNELS * sizeof(int));
  if (!temp_dst_out)
    return;

  int cache[16];

  int *temp_dst_rows[5] = {temp_dst_out,
                           temp_dst_out + (img->width * RGB_CHANNELS),
                           temp_dst_out + (2 * img->width * RGB_CHANNELS),
                           temp_dst_out + (3 * img->width * RGB_CHANNELS),
                           temp_dst_out + (4 * img->width * RGB_CHANNELS)};

  int s_y = -2;
  int e_y = 3;

  for (; y < end; y++) {

    for (; s_y < e_y; s_y++) {
      unsigned char *cur_src = rows[s_y + 2];
      int *temp_out = temp_dst_rows[s_y + 2];
      int x = 0;
      const unsigned char *src0 = cur_src;

      x = blur_1d_3c(x, min(6, img->width), cur_src, img->width, temp_out);
      temp_out = temp_out + (x * 6);

      const unsigned char *src0_1 = cur_src;
      const unsigned char *src0_2 = cur_src + 3;
      const unsigned char *src1 = cur_src + 6;
      const unsigned char *src2 = cur_src + 9;
      const unsigned char *src3 = cur_src + 12;
      for (; x <= img->width - 5; x += 5) {
        simde__m256i a = simde_mm256_cvtepu8_epi16(
            simde_mm_loadu_si128((const simde__m128i *)src0));

        simde__m256i b = simde_mm256_cvtepu8_epi16(
            simde_mm_loadu_si128((const simde__m128i *)src1));

        simde__m256i c = simde_mm256_cvtepu8_epi16(
            simde_mm_loadu_si128((const simde__m128i *)src2));

        simde__m256i d = simde_mm256_cvtepu8_epi16(
            simde_mm_loadu_si128((const simde__m128i *)src2));

        simde__m256i e = simde_mm256_cvtepu8_epi16(
            simde_mm_loadu_si128((const simde__m128i *)src2));

        simde__m256i sum = simde_mm256_add_epi16(
            a, simde_mm256_add_epi16(
                   b, simde_mm256_add_epi16(c, simde_mm256_add_epi16(d, e))));

        simde_mm256_storeu_si256((simde__m256i *)cache, sum);

        temp_out[0] = cache[0], temp_out[1] = cache[1], temp_out[2] = cache[2];
        temp_out[3] = cache[6], temp_out[4] = cache[7], temp_out[5] = cache[8];
        temp_out[6] = cache[12], temp_out[7] = cache[13],
        temp_out[8] = cache[14];

        temp_out += (5 * RGB_CHANNELS);
        src0 += (5 * RGB_CHANNELS), src1 += (5 * RGB_CHANNELS); src0_2 += (5 * RGB_CHANNELS);
        src2 += (5 * RGB_CHANNELS);src0_1 += (5 * RGB_CHANNELS);
      }

      blur_1d_3c(x, img->width, cur_src, img->width, temp_out);
    }

    unsigned char *out_row = img->data + (RGB_CHANNELS * img->width * y);

    int *row0 = temp_dst_rows[0], *row1 = temp_dst_rows[1],
        *row2 = temp_dst_rows[2], *row3 = temp_dst_rows[3],
        *row4 = temp_dst_rows[4];

    int xx = blur_1d_v_simd(0, img->width * 3, row0, row1, row2, row3, row4,
                            out_row);
    int x = xx / 3;
    out_row = out_row + (x * RGB_CHANNELS);

    for (; x < img->width; ++x) {
      int xx = x * RGB_CHANNELS;
      for (int c = 0; c < RGB_CHANNELS; c++) {
        out_row[0] = clamp((row0[xx + c] + row1[xx + c] + row2[xx + c] +
                            row3[xx + c] + row4[xx + c]) >>
                               4,
                           0, 255);

        ++out_row;
      }
    }

    rows[0] = rows[2], rows[1] = rows[3], rows[2] = rows[4];
    rows[3] = img->data + (reflect_index(((y + 1) * 2) + 1, img->height)) *
                              (img->width * RGB_CHANNELS);
    rows[4] = img->data + (reflect_index(((y + 1) * 2) + 2, img->height)) *
                              (img->width * RGB_CHANNELS);

    int *temp1 = temp_dst_rows[0], *temp2 = temp_dst_rows[1];
    temp_dst_rows[0] = temp_dst_rows[2], temp_dst_rows[1] = temp_dst_rows[3],
    temp_dst_rows[2] = temp_dst_rows[4], temp_dst_rows[3] = temp1,
    temp_dst_rows[4] = temp2;

    s_y = 1;
  }

  free(temp_dst_out);
}
