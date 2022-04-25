#include <cstdint>
#include <vector>

#pragma once

namespace bmpxx
{
  struct BmpDesc
  {
    int32_t width;
    int32_t height;
    uint8_t channels;

    BmpDesc(uint32_t width, uint32_t height, uint8_t channels)
        : width(width), height(height), channels(channels) {}
    BmpDesc() : width(0), height(0), channels(0) {}
  };

  class bmp
  {
  private:
    // ============================================================
    // Enums
    // ============================================================

    enum ColorSpace
    {
      CALIBRATED_RGB = 0x00000000,
      S_RGB = 0x73524742,
      WINDOWS_COLOR_SPACE = 0x57696E20,
      PROFILE_LINKED = 0x4C494E4B,
      PROFILE_EMBEDDED = 0x4D424544
    };

    enum GamutMappingIntent
    {
      GM_ABS_COLORIMETRIC = 0x00000008,
      GM_BUSINESS = 0x00000001,
      GM_GRAPHICS = 0x00000002,
      GM_IMAGES = 0x00000004
    };

    enum DibCompression
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

    struct DecodedRgbaMasks
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

    struct RgbaColor
    {
      uint8_t blue;
      uint8_t green;
      uint8_t red;
      uint8_t alpha;
    };

    struct DibHeaderMeta
    {
      uint32_t padded_row_width = 0;
      uint32_t expected_data_size = 0;
      uint8_t has_alpha_channel = 0;
      uint8_t has_icon_mask = 0;
    };

    // ============================================================
    // Packed structs
    // ============================================================

    struct __attribute__((packed)) Ciexyz
    {
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
    };

    struct __attribute__((packed)) CiexyxTriple
    {
      Ciexyz red = Ciexyz();
      Ciexyz green = Ciexyz();
      Ciexyz blue = Ciexyz();
    };

    struct __attribute__((packed)) BmpHeader
    {
      char identifier[2] = {'B', 'M'};
      uint32_t file_size = 0;
      uint16_t reserved1 = 0;
      uint16_t reserved2 = 0;
      uint32_t data_offset = 0;
    };

    struct __attribute__((packed)) RgbMasks
    {
      uint32_t red_mask = 0;
      uint32_t green_mask = 0;
      uint32_t blue_mask = 0;
    };

    struct __attribute__((packed)) RgbaMasks : RgbMasks
    {
      uint32_t alpha_mask = 0;
    };

    struct __attribute__((packed)) Dib12Header
    {
      uint32_t header_size = 0;
      uint16_t width = 0;
      uint16_t height = 0;
      uint16_t planes = 0;
      uint16_t bits_per_pixel = 0;
    };

    struct __attribute__((packed)) Dib40Header
    {
      uint32_t header_size = 0;
      std::int32_t width = 0;
      std::int32_t height = 0;
      uint16_t planes = 1;
      uint16_t bits_per_pixel = 0;
      DibCompression compression = BI_RGB;
      uint32_t data_size = 0;
      uint32_t horizontal_resolution = 96;
      uint32_t vertical_resolution = 96;
      uint32_t colors_used = 0;
      uint32_t important_colors = 0;
    };

    struct __attribute__((packed)) Dib52Header : Dib40Header
    {
      RgbMasks masks_rgb = RgbMasks();
    };

    struct __attribute__((packed)) Dib56Header : Dib40Header
    {
      RgbaMasks masks_rgba = RgbaMasks();
    };

    struct __attribute__((packed)) Dib108Header : Dib56Header
    {
      ColorSpace color_space_type = S_RGB;

      CiexyxTriple endpoints = CiexyxTriple();

      uint32_t gamma_red = 0;
      uint32_t gamma_green = 0;
      uint32_t gamma_blue = 0;
    };

    struct __attribute__((packed)) Dib124Header : Dib108Header
    {
      GamutMappingIntent intent = GM_GRAPHICS;
      uint32_t profile_data = 0;
      uint32_t profile_size = 0;
      uint32_t reserved = 0;
    };

    struct __attribute__((packed)) DibDecodeHeader : Dib124Header
    {
      DibHeaderMeta meta = DibHeaderMeta();
    };

    struct __attribute__((packed)) DibEncodeHeader : Dib108Header
    {
      DibHeaderMeta meta = DibHeaderMeta();

      DibEncodeHeader()
      {
        header_size = 108;
      }
    };

    // ============================================================
    // Private Methods
    // ============================================================

    // Decode

    static std::pair<std::vector<uint8_t>, BmpDesc> decode_headerless(
        std::vector<uint8_t> headerless_image,
        BmpHeader *bmp_header);
    static std::pair<std::vector<uint8_t>, BmpDesc> decodePalette(
        std::vector<uint8_t> inputImage,
        BmpHeader *bmp_header,
        DibDecodeHeader *dib_header);
    static std::pair<std::vector<uint8_t>, BmpDesc> decodeNormal(
        std::vector<uint8_t> inputImage,
        BmpHeader *bmp_header,
        DibDecodeHeader *dib_header);
    static DecodedRgbaMasks decodeMasks(DibDecodeHeader *dib_header);

    static std::pair<std::vector<uint8_t>, bmp::BmpHeader> readBMPHeader(std::vector<uint8_t> inputImage);
    static uint32_t readDIBHeaderSize(std::vector<uint8_t> inputImage, BmpHeader *bmp_header);
    static DibDecodeHeader readDIBHeader(std::vector<uint8_t> inputImage, BmpHeader *bmp_header);

    static void fixDIBHeaderDataSize(DibDecodeHeader *dib_header);
    static void fixDIBHeaderCompression(std::vector<uint8_t> inputImage, DibDecodeHeader *dib_header);
    static void fixDIBHeaderMasks(DibDecodeHeader *dib_header);

    // Encode

    static DibEncodeHeader createEncodeDibHeader(BmpDesc desc);

    // Shared

    static DibHeaderMeta createDIBHeaderMeta(Dib56Header *dib_header);

  public:
    static std::pair<std::vector<uint8_t>, BmpDesc> decode(std::vector<uint8_t> inputImage);
    static std::pair<std::vector<uint8_t>, BmpDesc> decode_icon(std::vector<uint8_t> inputImage);
    static std::vector<uint8_t> encode(std::vector<uint8_t> input, BmpDesc desc);
  };

  // Decode

}
