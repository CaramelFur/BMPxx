#include "bmpxx.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <type_traits>
#include <memory>
#include <stdexcept>
#include <bit>
#include <utility>
#include <iostream>
#include <iomanip>

namespace bmpxx
{
  std::pair<std::vector<byte>, bmp_desc> decode(std::vector<byte> inputImage)
  {
    // ===============
    // Main BMP header
    // ===============

    // Check if the input image is large enough to contain the main header.
    if (inputImage.size() <= sizeof(internal::bmp_header) + sizeof(internal::dib_12_header))
      throw std::runtime_error("input image is too small");

    auto header = reinterpret_cast<internal::bmp_header *>(inputImage.data());

    if (!(
            std::memcmp(header->identifier, "BM", 2) == 0 ||
            std::memcmp(header->identifier, "BA", 2) == 0 ||
            std::memcmp(header->identifier, "CI", 2) == 0 ||
            std::memcmp(header->identifier, "CP", 2) == 0 ||
            std::memcmp(header->identifier, "IC", 2) == 0 ||
            std::memcmp(header->identifier, "PT", 2) == 0))
      throw std::runtime_error("input image is not a BMP image");

    if (header->file_size != inputImage.size())
      throw std::runtime_error("input image size does not match file size");

    // First 4 bytes after BMP header are DIB header size
    const uint32_t dib_header_size = *reinterpret_cast<const uint32_t *>(inputImage.data() + sizeof(internal::bmp_header));

    // Check if the input image is large enough to contain the DIB header.
    if (inputImage.size() <= sizeof(internal::bmp_header) + dib_header_size)
      throw std::runtime_error("input image is too small");

    // Check if the data offset is valid
    if (header->data_offset < sizeof(internal::bmp_header) + dib_header_size)
      throw std::runtime_error("input image data offset is too small");

    if (header->data_offset > inputImage.size())
      throw std::runtime_error("input image data offset is too large");

    // ===============
    // Main DIB header
    // ===============

    auto dib_header = internal::dib_124_header(); // Pre populated

    switch (dib_header_size)
    {
    case sizeof(internal::dib_40_header):
    case sizeof(internal::dib_52_header):
    case sizeof(internal::dib_56_header):
    case sizeof(internal::dib_108_header):
    case sizeof(internal::dib_124_header):
    {
      std::memcpy(&dib_header, inputImage.data() + sizeof(internal::bmp_header), dib_header_size);
    }
    break;

    case sizeof(internal::dib_12_header):
    {
      auto dib_header12 = reinterpret_cast<const internal::dib_12_header *>(inputImage.data());
      dib_header.header_size = dib_header12->header_size;
      dib_header.width = dib_header12->width;
      dib_header.height = dib_header12->height;
      dib_header.planes = dib_header12->planes;
      dib_header.bits_per_pixel = dib_header12->bits_per_pixel;
    }
    break;

    default:
    {
      throw std::runtime_error("input image DIB header size is invalid");
    }
    }

    // Check that the image has a normal size
    if (dib_header.width <= 0 || dib_header.height <= 0)
      throw std::runtime_error("input image width or height is invalid");

    // This must always be one
    if (dib_header.planes != 1)
      throw std::runtime_error("input image planes is not 1");

    // Ensure it uses a compatible compression
    if (
        dib_header.compression != internal::BI_RGB &&
        dib_header.compression != internal::BI_BITFIELDS &&
        dib_header.compression != internal::BI_ALPHABITFIELDS)
      throw std::runtime_error("input image compression is not supported");

    if (dib_header.header_size <= sizeof(internal::dib_40_header))
    {
      // Merge bitfields into the dib header, since in some cases this is not done
      if (dib_header.compression == internal::BI_BITFIELDS)
      {
        auto rgb_masks = reinterpret_cast<internal::rgb_masks *>(inputImage.data() + sizeof(internal::bmp_header) + dib_header_size);
        auto target_masks = reinterpret_cast<internal::rgb_masks *>(&dib_header.masks_rgba);
        std::memcpy(target_masks, rgb_masks, sizeof(internal::rgb_masks));
        dib_header.header_size += sizeof(internal::rgb_masks);
      }
      else if (dib_header.compression == internal::BI_ALPHABITFIELDS)
      {
        auto rgba_masks = reinterpret_cast<const internal::rgba_masks *>(inputImage.data() + sizeof(internal::bmp_header) + dib_header_size);
        std::memcpy(&dib_header.masks_rgba, rgba_masks, sizeof(internal::rgba_masks));
        dib_header.header_size += sizeof(internal::rgba_masks);
      }
    }

    std::vector<uint32_t> pixels;

    switch (dib_header.bits_per_pixel)
    {
    case 1:
    case 2:
    case 4:
    case 8:
    {
      pixels = decodePalette(inputImage, header, &dib_header);
      break;
    }

    case 16:
    {
      pixels = decode16Bit(inputImage, header, &dib_header);
      break;
    }
    case 24:
    {
      pixels = decode24Bit(inputImage, header, &dib_header);
      break;
    }
    case 32:
    {
      pixels = decode32Bit(inputImage, header, &dib_header);
      break;
    }
    default:
    {
      throw std::runtime_error("input image bits per pixel is invalid");
    }
    }

    auto decodedMasks = decodeMasks(&dib_header);

    auto description = bmp_desc();
    description.width = dib_header.width;
    description.height = dib_header.height;
    if (decodedMasks.alpha_mask == 0)
      description.channels = 3;
    else
      description.channels = 4;

    std::vector<byte> output_pixels(description.width * description.height * description.channels);
    if (description.channels == 3)
    {
      for (int32_t y = 0; y < (int32_t)description.height; y++)
      {
        for (int32_t x = 0; x < (int32_t)description.width; x++)
        {
          uint32_t pixel = pixels[(description.height - y - 1) * description.width + x];

          // if (pixel != 0xffff)
          //   printf("pixel %x\n", pixel);

          float rf = (float)((pixel >> decodedMasks.red_shift) & decodedMasks.red_mask);
          float gf = (float)((pixel >> decodedMasks.green_shift) & decodedMasks.green_mask);
          float bf = (float)((pixel >> decodedMasks.blue_shift) & decodedMasks.blue_mask);

          byte r = (byte)(rf * 255.0f / decodedMasks.red_mask);
          byte g = (byte)(gf * 255.0f / decodedMasks.green_mask);
          byte b = (byte)(bf * 255.0f / decodedMasks.blue_mask);

          output_pixels[y * description.width * description.channels + x * description.channels + 0] = r;
          output_pixels[y * description.width * description.channels + x * description.channels + 1] = g;
          output_pixels[y * description.width * description.channels + x * description.channels + 2] = b;
        }
      }
    }
    else
    {
      for (int32_t y = 0; y < (int32_t)description.height; y++)
      {
        for (int32_t x = 0; x < (int32_t)description.width; x++)
        {
          uint32_t pixel = pixels[(description.height - y - 1) * description.width + x];
          float rf = (float)((pixel >> decodedMasks.red_shift) & decodedMasks.red_mask);
          float gf = (float)((pixel >> decodedMasks.green_shift) & decodedMasks.green_mask);
          float bf = (float)((pixel >> decodedMasks.blue_shift) & decodedMasks.blue_mask);
          float af = (float)((pixel >> decodedMasks.alpha_shift) & decodedMasks.alpha_mask);

          byte r = (byte)(rf * 255.0f / decodedMasks.red_mask);
          byte g = (byte)(gf * 255.0f / decodedMasks.green_mask);
          byte b = (byte)(bf * 255.0f / decodedMasks.blue_mask);
          byte a = (byte)(af * 255.0f / decodedMasks.alpha_mask);

          output_pixels[y * description.width * description.channels + x * description.channels + 0] = r;
          output_pixels[y * description.width * description.channels + x * description.channels + 1] = g;
          output_pixels[y * description.width * description.channels + x * description.channels + 2] = b;
          output_pixels[y * description.width * description.channels + x * description.channels + 3] = a;
        }
      }
    }

    return std::make_pair(output_pixels, description);
  }

  std::vector<uint32_t> decodePalette(
      std::vector<byte> inputImage,
      internal::bmp_header *header,
      internal::dib_124_header *dib_header)
  {
    const uint32_t row_width_full_bytes = calculatePaddedRowWidthAndDatasize(dib_header);
    if (inputImage.size() < header->data_offset + dib_header->data_size)
      throw std::runtime_error("input image too small");

    if (dib_header->colors_used == 0 || dib_header->colors_used > 256)
      throw std::runtime_error("input image colors used is invalid");

    // Check if the data_offset is actually a valid position in the file
    if (header->data_offset < sizeof(internal::bmp_header) + dib_header->header_size + dib_header->colors_used * 4)
      throw std::runtime_error("input image data offset is too small");

    // Extract pointer to palette and data
    const uint32_t *palette = reinterpret_cast<const uint32_t *>(inputImage.data() + sizeof(internal::bmp_header) + dib_header->header_size);

    const uint32_t pixels_per_byte = (8 / dib_header->bits_per_pixel);
    const uint32_t pixels_mask = ((1 << dib_header->bits_per_pixel) - 1);

    std::vector<uint32_t> decoded_data(dib_header->width * dib_header->height);

    for (int32_t y = 0; y < dib_header->height; y++)
    {
      const byte *row_ptr = reinterpret_cast<const byte *>(inputImage.data() + header->data_offset + y * row_width_full_bytes);
      for (int32_t x = 0; x < dib_header->width; x++)
      {
        const uint32_t target_byte = x / pixels_per_byte;
        const uint32_t target_bit = x % pixels_per_byte;

        const uint32_t target_shift = 8 - ((target_bit + 1) * dib_header->bits_per_pixel);

        const uint32_t palette_index = (row_ptr[target_byte] >> target_shift) & pixels_mask;
        decoded_data[y * dib_header->width + x] = palette[palette_index];
      }
    }

    return decoded_data;
  }

  std::vector<uint32_t> decode16Bit(std::vector<byte> inputImage,
                                    internal::bmp_header *header,
                                    internal::dib_124_header *dib_header)
  {
    const uint32_t row_width_full_bytes = calculatePaddedRowWidthAndDatasize(dib_header);
    if (inputImage.size() < header->data_offset + dib_header->data_size)
      throw std::runtime_error("input image too small");

    std::vector<uint32_t> decoded_data(dib_header->width * dib_header->height);

    for (int32_t y = 0; y < dib_header->height; y++)
    {
      const uint16_t *row_ptr = reinterpret_cast<const uint16_t *>(inputImage.data() + header->data_offset + y * row_width_full_bytes);
      for (int32_t x = 0; x < dib_header->width; x++)
      {
        decoded_data[y * dib_header->width + x] = row_ptr[x];
      }
    }

    return decoded_data;
  }

  std::vector<uint32_t> decode24Bit(std::vector<byte> inputImage,
                                    internal::bmp_header *header,
                                    internal::dib_124_header *dib_header)
  {
    const uint32_t row_width_full_bytes = calculatePaddedRowWidthAndDatasize(dib_header);
    if (inputImage.size() < header->data_offset + dib_header->data_size)
      throw std::runtime_error("input image too small");

    std::vector<uint32_t> decoded_data(dib_header->width * dib_header->height);

    for (int32_t y = 0; y < dib_header->height; y++)
    {

      for (int32_t x = 0; x < dib_header->width; x++)
      {
        const uint32_t *pixl_pointer = reinterpret_cast<const uint32_t *>(inputImage.data() + header->data_offset + y * row_width_full_bytes + x * 3);
        decoded_data[y * dib_header->width + x] = *pixl_pointer & 0x00FFFFFF;
      }
    }

    return decoded_data;
  }

  std::vector<uint32_t> decode32Bit(std::vector<byte> inputImage,
                                    internal::bmp_header *header,
                                    internal::dib_124_header *dib_header)
  {
    const uint32_t row_width_full_bytes = calculatePaddedRowWidthAndDatasize(dib_header);
    if (inputImage.size() < header->data_offset + dib_header->data_size)
      throw std::runtime_error("input image too small");

    std::vector<uint32_t> decoded_data(dib_header->width * dib_header->height);

    for (int32_t y = 0; y < dib_header->height; y++)
    {
      const uint32_t *row_ptr = reinterpret_cast<const uint32_t *>(inputImage.data() + header->data_offset + y * row_width_full_bytes);
      for (int32_t x = 0; x < dib_header->width; x++)
      {
        decoded_data[y * dib_header->width + x] = row_ptr[x];
      }
    }

    return decoded_data;
  }

  internal::decoded_rgba_masks decodeMasks(internal::dib_124_header *dib_header)
  {
    auto masks = internal::decoded_rgba_masks();

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

    return masks;
  }

  uint32_t calculatePaddedRowWidthAndDatasize(internal::dib_124_header *dib_header)
  {
    const uint32_t row_width_bits = dib_header->width * dib_header->bits_per_pixel;
    const uint32_t row_padding_bits = (32 - (row_width_bits % 32)) % 32;
    const uint32_t row_width_full_bytes = (row_width_bits + row_padding_bits) / 8;

    // Calculate the expected size of the image data, anc check if it matches
    {
      const uint32_t expected_data_size =
          dib_header->height * (row_width_bits + row_padding_bits) / 8;

      if (dib_header->data_size != 0)
        dib_header->data_size = expected_data_size;

      if (dib_header->data_size != expected_data_size)
        throw std::runtime_error("input image size does not match expected image size");
    }

    return row_width_full_bytes;
  }
}
