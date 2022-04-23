#include <cstdint>
#include <utility>
#include <vector>

#pragma once

namespace bmpxx
{
  typedef uint8_t byte;

  namespace internal
  {
    enum color_space
    {
      CALIBRATED_RGB = 0x00000000,
      S_RGB = 0x73524742,
      WINDOWS_COLOR_SPACE = 0x57696E20,
      PROFILE_LINKED = 0x4C494E4B,
      PROFILE_EMBEDDED = 0x4D424544
    };

    enum gamut_mapping_intent
    {
      GM_ABS_COLORIMETRIC = 0x00000008,
      GM_BUSINESS = 0x00000001,
      GM_GRAPHICS = 0x00000002,
      GM_IMAGES = 0x00000004
    };

    enum dib_compression
    {
      BI_RGB = 0,
      BI_RLE8 = 1,
      BI_RLE4 = 2,
      BI_BITFIELDS = 3,
      BI_JPEG = 4,
      BI_PNG = 5,
      BI_ALPHABITFIELDS = 6,
      BI_CMYK = 11,
      BI_CMYKRLE8 = 12,
      BI_CMYKRLE4 = 13
    };

    struct decoded_rgba_masks
    {
      uint8_t red_mask = 0;
      uint8_t green_mask = 0;
      uint8_t blue_mask = 0;
      uint8_t alpha_mask = 0;

      uint8_t red_shift = 0;
      uint8_t green_shift = 0;
      uint8_t blue_shift = 0;
      uint8_t alpha_shift = 0;
    };

    struct __attribute__((packed)) ciexyz
    {
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
    };

    struct __attribute__((packed)) ciexyx_triple
    {
      ciexyz red = ciexyz();
      ciexyz green = ciexyz();
      ciexyz blue = ciexyz();
    };

    struct __attribute__((packed)) bmp_header
    {
      char identifier[2] = {'B', 'M'};
      uint32_t file_size = 0;
      uint16_t reserved1 = 0;
      uint16_t reserved2 = 0;
      uint32_t data_offset = 0;
    };

    struct __attribute__((packed)) rgb_masks
    {
      uint32_t red_mask = 0;
      uint32_t green_mask = 0;
      uint32_t blue_mask = 0;
    };

    struct __attribute__((packed)) rgba_masks : rgb_masks
    {
      uint32_t alpha_mask = 0;
    };

    struct __attribute__((packed)) dib_12_header
    {
      uint32_t header_size = 0;
      uint16_t width = 0;
      uint16_t height = 0;
      uint16_t planes = 0;
      uint16_t bits_per_pixel = 0;
    };

    struct __attribute__((packed)) dib_40_header
    {
      uint32_t header_size = 0;
      std::int32_t width = 0;
      std::int32_t height = 0;
      uint16_t planes = 0;
      uint16_t bits_per_pixel = 0;
      dib_compression compression = BI_RGB;
      uint32_t data_size = 0;
      uint32_t horizontal_resolution = 96;
      uint32_t vertical_resolution = 96;
      uint32_t colors_used = 0;
      uint32_t important_colors = 0;
    };

    struct __attribute__((packed)) dib_52_header : dib_40_header
    {
      rgb_masks masks_rgb = rgb_masks();
    };

    struct __attribute__((packed)) dib_56_header : dib_40_header
    {
      rgba_masks masks_rgba = rgba_masks();
    };

    struct __attribute__((packed)) dib_108_header : dib_56_header
    {
      color_space color_space_type = S_RGB;

      ciexyx_triple endpoints = ciexyx_triple();

      uint32_t gamma_red = 0;
      uint32_t gamma_green = 0;
      uint32_t gamma_blue = 0;
    };

    struct __attribute__((packed)) dib_124_header : dib_108_header
    {
      gamut_mapping_intent intent = GM_GRAPHICS;
      uint32_t profile_data = 0;
      uint32_t profile_size = 0;
      uint32_t reserved = 0;
    };
  }

  struct bmp_desc
  {
    uint32_t width;
    uint32_t height;
    uint8_t channels;
  };

  std::pair<std::vector<byte>, bmp_desc> decode(std::vector<byte> inputImage);
  std::vector<uint32_t> decodePalette(
      std::vector<byte> inputImage,
      internal::bmp_header *header,
      internal::dib_124_header *dib_header);
  std::vector<uint32_t> decode16Bit(
      std::vector<byte> inputImage,
      internal::bmp_header *header,
      internal::dib_124_header *dib_header);
  std::vector<uint32_t> decode24Bit(
      std::vector<byte> inputImage,
      internal::bmp_header *header,
      internal::dib_124_header *dib_header);
  std::vector<uint32_t> decode32Bit(
      std::vector<byte> inputImage,
      internal::bmp_header *header,
      internal::dib_124_header *dib_header);
  internal::decoded_rgba_masks decodeMasks(internal::dib_124_header *dib_header);
  uint32_t calculatePaddedRowWidthAndDatasize(internal::dib_124_header *dib_header);
}
