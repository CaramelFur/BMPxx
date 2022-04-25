#include "bmpxx.hpp"

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <bit>

namespace bmpxx
{
  bmp::DibHeaderMeta bmp::createDIBHeaderMeta(Dib56Header *dib_header)
  {
    auto dib_header_meta = DibHeaderMeta();

    // Padding rows
    const uint32_t row_width_bits = dib_header->width * dib_header->bits_per_pixel;
    const uint32_t row_padding_bits = (32 - (row_width_bits % 32)) % 32;
    const uint32_t padded_row_width_bytes = (row_width_bits + row_padding_bits) / 8;

    const uint32_t expected_data_size = dib_header->height * padded_row_width_bytes;

    dib_header_meta.padded_row_width = padded_row_width_bytes;
    dib_header_meta.expected_data_size = expected_data_size;
    // Alpha
    dib_header_meta.has_alpha_channel = dib_header->masks_rgba.alpha_mask != 0;

    if (dib_header->data_size * 2 == dib_header_meta.expected_data_size)
    {
      dib_header_meta.expected_data_size /= 2;
      dib_header_meta.has_icon_mask = true;
    }

    return dib_header_meta;
  }
}
