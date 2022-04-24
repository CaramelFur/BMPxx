#include "bmpxx.hpp"

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <bit>

namespace bmpxx
{
  std::pair<std::vector<uint8_t>, BmpDesc> bmp::decode(std::vector<uint8_t> inputImage)
  {
    auto bmp_header = readBMPHeader(inputImage);

    auto dib_header = readDIBHeader(inputImage, &bmp_header);

    switch (dib_header.bits_per_pixel)
    {
    case 1:
    case 2:
    case 4:
    case 8:
    {
      return decodePalette(inputImage, &bmp_header, &dib_header);
      break;
    }

    case 16:
    case 24:
    case 32:
    {
      return decodeNormal(inputImage, &bmp_header, &dib_header);
      break;
    }

    default:
    {
      throw std::runtime_error("input image bits per pixel is invalid");
    }
    }
  }

  std::pair<std::vector<uint8_t>, BmpDesc> bmp::decodePalette(
      std::vector<uint8_t> inputImage,
      BmpHeader *bmp_header,
      DibDecodeHeader *dib_header)
  {
    if (dib_header->colors_used == 0 || dib_header->colors_used > 256)
      throw std::runtime_error("input image colors used is invalid");

    // Check if the data_offset is actually a valid position in the file
    if (bmp_header->data_offset < sizeof(BmpHeader) + dib_header->header_size + dib_header->colors_used * 4)
      throw std::runtime_error("input image data offset is too small");

    // Extract pointer to palette
    const uint32_t *palette = reinterpret_cast<const uint32_t *>(inputImage.data() + sizeof(BmpHeader) + dib_header->header_size);

    const uint32_t pixels_per_byte = (8 / dib_header->bits_per_pixel);
    const uint32_t pixels_mask = ((1 << dib_header->bits_per_pixel) - 1);

    // A palette image is always 3 channel RGB
    uint32_t decoded_data_pos = 0;
    auto description = BmpDesc(dib_header->width, dib_header->height, 3);
    std::vector<uint8_t> decoded_data(description.width * description.height * description.channels);

    for (int32_t y = dib_header->height - 1; y >= 0; y--)
    {
      const uint8_t *row_ptr = reinterpret_cast<const uint8_t *>(inputImage.data() + bmp_header->data_offset + y * dib_header->meta.padded_row_width);
      for (int32_t x = 0; x < dib_header->width; x++)
      {
        const uint32_t target_byte = x / pixels_per_byte;
        const uint32_t target_bit = x % pixels_per_byte;

        const uint32_t target_shift = 8 - ((target_bit + 1) * dib_header->bits_per_pixel);

        const uint32_t palette_index = (row_ptr[target_byte] >> target_shift) & pixels_mask;

        const RgbaColor *color = reinterpret_cast<const RgbaColor *>(palette + palette_index);

        decoded_data[decoded_data_pos++] = color->red;
        decoded_data[decoded_data_pos++] = color->green;
        decoded_data[decoded_data_pos++] = color->blue;
      }
    }

    return std::make_pair(decoded_data, description);
  }

  std::pair<std::vector<uint8_t>, BmpDesc> bmp::decodeNormal(
      std::vector<uint8_t> inputImage,
      BmpHeader *bmp_header,
      DibDecodeHeader *dib_header)
  {
    uint32_t decoded_data_pos = 0;
    auto description = BmpDesc(
        dib_header->width,
        dib_header->height,
        dib_header->meta.has_alpha_channel ? 4 : 3);
    std::vector<uint8_t> decoded_data(description.width * description.height * description.channels);

    auto masks = decodeMasks(dib_header);
    // Should be divisible by 8 if reached here, not gonna check for speed
    uint32_t bytes_per_pixel = dib_header->bits_per_pixel / 8;

    for (int32_t y = dib_header->height - 1; y >= 0; y--)
    {

      for (int32_t x = 0; x < dib_header->width; x++)
      {
        const uint32_t *pixel_ptr = reinterpret_cast<const uint32_t *>(inputImage.data() + bmp_header->data_offset + y * dib_header->meta.padded_row_width + x * bytes_per_pixel);

        const float red_unscaled = (float)((*pixel_ptr >> masks.red_shift) & masks.red_mask);
        decoded_data[decoded_data_pos++] = (uint8_t)(red_unscaled * masks.red_scale);

        const float green_unscaled = (float)((*pixel_ptr >> masks.green_shift) & masks.green_mask);
        decoded_data[decoded_data_pos++] = (uint8_t)(green_unscaled * masks.green_scale);

        const float blue_unscaled = (float)((*pixel_ptr >> masks.blue_shift) & masks.blue_mask);
        decoded_data[decoded_data_pos++] = (uint8_t)(blue_unscaled * masks.blue_scale);

        if (dib_header->meta.has_alpha_channel)
        {
          const float alpha_unscaled = (float)((*pixel_ptr >> masks.alpha_shift) & masks.alpha_mask);
          decoded_data[decoded_data_pos++] = (uint8_t)(alpha_unscaled * masks.alpha_scale);
        }
      }
    }

    return std::make_pair(decoded_data, description);
  }

  bmp::DecodedRgbaMasks bmp::decodeMasks(DibDecodeHeader *dib_header)
  {
    auto masks = DecodedRgbaMasks();

    int red_rzero = std::countr_zero(dib_header->masks_rgba.red_mask) % 32;
    int green_rzero = std::countr_zero(dib_header->masks_rgba.green_mask) % 32;
    int blue_rzero = std::countr_zero(dib_header->masks_rgba.blue_mask) % 32;
    int alpha_rzero = std::countr_zero(dib_header->masks_rgba.alpha_mask) % 32;

    int red_width = 32 - std::countl_zero(dib_header->masks_rgba.red_mask) - red_rzero;
    int green_width = 32 - std::countl_zero(dib_header->masks_rgba.green_mask) - green_rzero;
    int blue_width = 32 - std::countl_zero(dib_header->masks_rgba.blue_mask) - blue_rzero;
    int alpha_width = 32 - std::countl_zero(dib_header->masks_rgba.alpha_mask) - alpha_rzero;

    int red_temp_shift = red_width <= 8 ? red_rzero : red_rzero + red_width - 8;
    int green_temp_shift = green_width <= 8 ? green_rzero : green_rzero + green_width - 8;
    int blue_temp_shift = blue_width <= 8 ? blue_rzero : blue_rzero + blue_width - 8;
    int alpha_temp_shift = alpha_width <= 8 ? alpha_rzero : alpha_rzero + alpha_width - 8;

    masks.red_shift = (uint8_t)(red_temp_shift < 0 ? 0 : red_temp_shift);
    masks.green_shift = (uint8_t)(green_temp_shift < 0 ? 0 : green_temp_shift);
    masks.blue_shift = (uint8_t)(blue_temp_shift < 0 ? 0 : blue_temp_shift);
    masks.alpha_shift = (uint8_t)(alpha_temp_shift < 0 ? 0 : alpha_temp_shift);

    masks.red_mask = (uint8_t)((1 << red_width) - 1);
    masks.green_mask = (uint8_t)((1 << green_width) - 1);
    masks.blue_mask = (uint8_t)((1 << blue_width) - 1);
    masks.alpha_mask = (uint8_t)((1 << alpha_width) - 1);

    masks.red_scale = 255.0f / masks.red_mask;
    masks.green_scale = 255.0f / masks.green_mask;
    masks.blue_scale = 255.0f / masks.blue_mask;
    masks.alpha_scale = 255.0f / masks.alpha_mask;

    return masks;
  }

  bmp::BmpHeader bmp::readBMPHeader(std::vector<uint8_t> inputImage)
  {
    // Check if the input image is large enough to contain the main header.
    if (inputImage.size() <= sizeof(BmpHeader) + sizeof(Dib12Header))
      throw std::runtime_error("input image is too small");

    auto header = BmpHeader();
    std::memcpy(&header, inputImage.data(), sizeof(BmpHeader));

    if (!(
            std::memcmp(header.identifier, "BM", 2) == 0 ||
            std::memcmp(header.identifier, "BA", 2) == 0 ||
            std::memcmp(header.identifier, "CI", 2) == 0 ||
            std::memcmp(header.identifier, "CP", 2) == 0 ||
            std::memcmp(header.identifier, "IC", 2) == 0 ||
            std::memcmp(header.identifier, "PT", 2) == 0))
      throw std::runtime_error("input image is not a BMP image");

    if (header.file_size != inputImage.size())
      throw std::runtime_error("input image size does not match file size");

    return header;
  }

  uint32_t bmp::readDIBHeaderSize(std::vector<uint8_t> inputImage, BmpHeader *bmp_header)
  {
    // First 4 bytes after BMP header are DIB header size
    const uint32_t dib_header_size = *reinterpret_cast<const uint32_t *>(inputImage.data() + sizeof(BmpHeader));

    // Check if the input image is large enough to contain the DIB header.
    if (inputImage.size() <= sizeof(BmpHeader) + dib_header_size)
      throw std::runtime_error("input image is too small");

    // Check if the data offset is valid
    if (bmp_header->data_offset < sizeof(BmpHeader) + dib_header_size)
      throw std::runtime_error("input image data offset is too small");

    if (bmp_header->data_offset > inputImage.size())
      throw std::runtime_error("input image data offset is too large");

    return dib_header_size;
  }

  bmp::DibDecodeHeader bmp::readDIBHeader(std::vector<uint8_t> inputImage, BmpHeader *bmp_header)
  {
    auto dib_header_size = readDIBHeaderSize(inputImage, bmp_header);

    auto dib_header = DibDecodeHeader(); // Pre populated

    switch (dib_header_size)
    {
      // These can all be copied directly, as they all overlap
    case sizeof(Dib40Header):
    case sizeof(Dib52Header):
    case sizeof(Dib56Header):
    case sizeof(Dib108Header):
    case sizeof(Dib124Header):
    {
      std::memcpy(&dib_header, inputImage.data() + sizeof(BmpHeader), dib_header_size);
      break;
    }
    case sizeof(Dib12Header):
    {
      // DIB 12 is the only outlier, we need to map its values
      auto dib_header12 = reinterpret_cast<const Dib12Header *>(inputImage.data());
      dib_header.header_size = dib_header12->header_size;
      dib_header.width = dib_header12->width;
      dib_header.height = dib_header12->height;
      dib_header.planes = dib_header12->planes;
      dib_header.bits_per_pixel = dib_header12->bits_per_pixel;
      break;
    }
    default:
    {
      throw std::runtime_error("input image DIB header size is invalid");
    }
    }

    // This order is important
    fixDIBHeaderCompression(inputImage, &dib_header);
    fixDIBHeaderMasks(&dib_header);

    dib_header.meta = createDIBHeaderMeta(&dib_header);
    fixDIBHeaderDataSize(&dib_header);

    if (inputImage.size() < bmp_header->data_offset + dib_header.data_size)
      throw std::runtime_error("input image is too small");

    // Check that the image has a normal size
    if (dib_header.width <= 0 || dib_header.height <= 0)
      throw std::runtime_error("input image width or height is invalid");

    // This must always be one
    if (dib_header.planes != 1)
      throw std::runtime_error("input image planes is not 1");

    return dib_header;
  }

  void bmp::fixDIBHeaderDataSize(DibDecodeHeader *dib_header)
  {
    const uint32_t expected_data_size = dib_header->height * dib_header->meta.padded_row_width;

    if (dib_header->data_size == 0)
      dib_header->data_size = expected_data_size;

    if (dib_header->data_size != expected_data_size)
      throw std::runtime_error("input image size does not match expected image size");
  }

  void bmp::fixDIBHeaderCompression(std::vector<uint8_t> inputImage, DibDecodeHeader *dib_header)
  {
    // Ensure it uses a compatible compression
    if (
        dib_header->compression != BI_RGB &&
        dib_header->compression != BI_BITFIELDS &&
        dib_header->compression != BI_ALPHABITFIELDS)
      throw std::runtime_error("input image compression is not supported");

    if (dib_header->header_size <= sizeof(Dib40Header))
    {
      // Merge bitfields into the dib header, since in some cases this is not done
      if (dib_header->compression == BI_BITFIELDS)
      {
        auto rgb_masks = reinterpret_cast<RgbMasks *>(inputImage.data() + sizeof(BmpHeader) + dib_header->header_size);
        auto target_masks = reinterpret_cast<RgbMasks *>(&dib_header->masks_rgba);
        std::memcpy(target_masks, rgb_masks, sizeof(RgbMasks));
        dib_header->header_size += sizeof(RgbMasks);
      }
      else if (dib_header->compression == BI_ALPHABITFIELDS)
      {
        auto rgba_masks = reinterpret_cast<const RgbaMasks *>(inputImage.data() + sizeof(BmpHeader) + dib_header->header_size);
        auto target_masks = reinterpret_cast<RgbaMasks *>(&dib_header->masks_rgba);
        std::memcpy(target_masks, rgba_masks, sizeof(RgbaMasks));
        dib_header->header_size += sizeof(RgbaMasks);
      }
    }
  }

  void bmp::fixDIBHeaderMasks(DibDecodeHeader *dib_header)
  {
    if (!(dib_header->masks_rgba.red_mask ||
          dib_header->masks_rgba.green_mask ||
          dib_header->masks_rgba.blue_mask))
    {
      if (dib_header->bits_per_pixel == 16)
      {
        // RGB555
        dib_header->masks_rgba.red_mask = 0x7c00;  // 0b0111110000000000
        dib_header->masks_rgba.green_mask = 0x3e0; // 0b0000001111100000
        dib_header->masks_rgba.blue_mask = 0x1f;   // 0b0000000000011111
      }
      else
      {
        // RGB888
        dib_header->masks_rgba.red_mask = 0x00ff0000;
        dib_header->masks_rgba.green_mask = 0x0000ff00;
        dib_header->masks_rgba.blue_mask = 0x000000ff;
      }
    }

    const uint32_t full_bit_mask = 0xffffffff >> (32 - dib_header->bits_per_pixel);

    dib_header->masks_rgba.red_mask &= full_bit_mask;
    dib_header->masks_rgba.green_mask &= full_bit_mask;
    dib_header->masks_rgba.blue_mask &= full_bit_mask;
    dib_header->masks_rgba.alpha_mask &= full_bit_mask;
  }
}
