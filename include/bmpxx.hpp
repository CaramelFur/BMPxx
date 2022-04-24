#include <cstdint>
#include <vector>

#pragma once

namespace bmpxx
{
  struct bmp_desc
  {
    int32_t width;
    int32_t height;
    uint8_t channels;

    bmp_desc(uint32_t width, uint32_t height, uint8_t channels)
        : width(width), height(height), channels(channels) {}
  };

  class bmp
  {
  private:
    // ============================================================
    // Enums
    // ============================================================

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

    // ============================================================
    // Structs
    // ============================================================

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

      float red_scale = 0.0f;
      float green_scale = 0.0f;
      float blue_scale = 0.0f;
      float alpha_scale = 0.0f;
    };

    struct RGBAColor
    {
      uint8_t red;
      uint8_t green;
      uint8_t blue;
      uint8_t alpha;
    };

    struct dib_header_meta
    {
      uint32_t padded_row_width = 0;
      uint8_t has_alpha_channel = 0;
    };

    // ============================================================
    // Packed structs
    // ============================================================

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
      uint16_t planes = 1;
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

    struct __attribute__((packed)) dib_decode_header : dib_124_header
    {
      dib_header_meta meta = dib_header_meta();
    };

    struct __attribute__((packed)) dib_encode_header : dib_56_header
    {
      dib_header_meta meta = dib_header_meta();

      dib_encode_header()
      {
        header_size = 56;
      }
    };

    // ============================================================
    // Private Methods
    // ============================================================

    // Decode

    static std::pair<std::vector<uint8_t>, bmp_desc> decodePalette(
        std::vector<uint8_t> inputImage,
        bmp_header *bmp_header,
        dib_decode_header *dib_header);
    static std::pair<std::vector<uint8_t>, bmp_desc> decodeNormal(
        std::vector<uint8_t> inputImage,
        bmp_header *bmp_header,
        dib_decode_header *dib_header);
    static decoded_rgba_masks decodeMasks(dib_decode_header *dib_header);

    static bmp_header readBMPHeader(std::vector<uint8_t> inputImage);
    static uint32_t readDIBHeaderSize(std::vector<uint8_t> inputImage, bmp_header *bmp_header);
    static dib_decode_header readDIBHeader(std::vector<uint8_t> inputImage, bmp_header *bmp_header);

    static void fixDIBHeaderDataSize(dib_decode_header *dib_header);
    static void fixDIBHeaderCompression(std::vector<uint8_t> inputImage, dib_decode_header *dib_header);
    static void fixDIBHeaderMasks(dib_decode_header *dib_header);

    // Encode

    static dib_encode_header createEncodeDibHeader(bmp_desc desc);

    // Shared

    static dib_header_meta createDIBHeaderMeta(dib_56_header *dib_header);

  public:
    static std::pair<std::vector<uint8_t>, bmp_desc> decode(std::vector<uint8_t> inputImage);
    static std::vector<uint8_t> encode(std::vector<uint8_t> input, bmp_desc desc);
  };

  // Decode

}
