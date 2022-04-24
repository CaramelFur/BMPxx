#include "bmpxx.hpp"

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <bit>

namespace bmpxx
{
  std::vector<uint8_t> encode(std::vector<uint8_t> input, bmp_desc desc)
  {
    if (desc.channels != 3 && desc.channels != 4)
    {
      throw std::runtime_error("Only 3 and 4 channels are supported");
    }

    const uint32_t expectedInputLength = desc.height * desc.width * desc.channels;
    if (input.size() != expectedInputLength)
    {
      throw std::invalid_argument("Input data size does not match the expected size.");
    }

    auto dib_header = internal::dib_56_header();

    dib_header.header_size = sizeof(dib_header);
    dib_header.width = desc.width;
    dib_header.height = desc.height;

    if (desc.channels == 4)
    {
      dib_header.bits_per_pixel = 32;
      dib_header.compression = internal::BI_BITFIELDS;
      dib_header.masks_rgba.red_mask = 0x00ff0000;
      dib_header.masks_rgba.green_mask = 0x0000ff00;
      dib_header.masks_rgba.blue_mask = 0x000000ff;
      dib_header.masks_rgba.alpha_mask = 0xff000000;
    }
    else
    {
      dib_header.bits_per_pixel = 24;
      dib_header.compression = internal::BI_RGB;
    }

    const uint32_t row_width_bits = dib_header.width * dib_header.bits_per_pixel;
    const uint32_t row_padding_bits = (32 - (row_width_bits % 32)) % 32;
    const uint32_t padded_row_width_bytes = (row_width_bits + row_padding_bits) / 8;

    dib_header.data_size = padded_row_width_bytes * desc.height;

    uint32_t output_pos = sizeof(internal::bmp_header) + sizeof(dib_header);
    std::vector<uint8_t> output(sizeof(internal::bmp_header) + sizeof(dib_header) + dib_header.data_size);

    const int32_t desc_bytes_width = desc.width * (int32_t)desc.channels;
    for (int32_t y = desc.height - 1; y >= 0; y--)
    {
      std::memcpy(&output[output_pos], &input[y * desc_bytes_width], desc_bytes_width);
      output_pos += padded_row_width_bytes;
    }

    auto bmp_header = internal::bmp_header();
    bmp_header.file_size = (uint32_t)output.size();
    bmp_header.data_offset = sizeof(internal::bmp_header) + sizeof(dib_header);

    std::memcpy(output.data(), &bmp_header, sizeof(internal::bmp_header));
    std::memcpy(output.data() + sizeof(internal::bmp_header), &dib_header, sizeof(dib_header));

    return output;
  }
}
