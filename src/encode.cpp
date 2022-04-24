#include "bmpxx.hpp"

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <bit>

namespace bmpxx
{
  std::vector<uint8_t> bmp::encode(std::vector<uint8_t> input, bmp_desc desc)
  {
    if (desc.channels != 3 && desc.channels != 4)
      throw std::runtime_error("Only 3 and 4 channels are supported");

    const uint32_t input_row_length = desc.width * desc.channels;
    const uint32_t expected_input_length = desc.height * input_row_length;
    if (input.size() != expected_input_length)
      throw std::invalid_argument("Input data size does not match the expected size.");

    auto dib_header = createEncodeDibHeader(desc);

    std::vector<uint8_t> output(sizeof(bmp_header) + dib_header.header_size + dib_header.data_size);
    uint32_t output_pos = sizeof(bmp_header) + dib_header.header_size;
    uint32_t input_pos = (uint32_t)input.size() - input_row_length;

    for (int32_t y = dib_header.height - 1; y >= 0; y--)
    {
      for (int32_t x = 0; x < (int32_t)input_row_length; x += desc.channels)
      {
        output[output_pos + x + 2] = input[input_pos + x + 0];
        output[output_pos + x + 1] = input[input_pos + x + 1];
        output[output_pos + x + 0] = input[input_pos + x + 2];

        if (desc.channels == 4)
          output[output_pos + x + 3] = input[input_pos + x + 3];
      }
      output_pos += dib_header.meta.padded_row_width;
      input_pos -= input_row_length;
    }

    auto bmp_head = bmp_header();
    bmp_head.file_size = (uint32_t)output.size();
    bmp_head.data_offset = sizeof(bmp_header) + dib_header.header_size;

    std::memcpy(output.data(), &bmp_head, sizeof(bmp_header));
    std::memcpy(output.data() + sizeof(bmp_header), &dib_header, dib_header.header_size);

    return output;
  }

  bmp::dib_encode_header bmp::createEncodeDibHeader(bmp_desc desc)
  {
    if (desc.channels != 3 && desc.channels != 4)
      throw std::runtime_error("Only 3 and 4 channels are supported");

    auto dib_header = dib_encode_header();

    dib_header.width = desc.width;
    dib_header.height = desc.height;

    if (desc.channels == 4)
    {
      dib_header.bits_per_pixel = 32;
      dib_header.compression = BI_BITFIELDS;
      dib_header.masks_rgba.red_mask = 0x00ff0000;
      dib_header.masks_rgba.green_mask = 0x0000ff00;
      dib_header.masks_rgba.blue_mask = 0x000000ff;
      dib_header.masks_rgba.alpha_mask = 0xff000000;
    }
    else
    {
      dib_header.bits_per_pixel = 24;
      dib_header.compression = BI_RGB;
    }

    dib_header.meta = createDIBHeaderMeta(&dib_header);
    dib_header.data_size = dib_header.meta.padded_row_width * desc.height;

    return dib_header;
  }

}
