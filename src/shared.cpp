#include "bmpxx.hpp"

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <bit>

namespace bmpxx
{
  bmp::dib_header_meta bmp::createDIBHeaderMeta(dib_56_header *dib_header)
  {
    auto dib_header_metadata = dib_header_meta();

    // Padding rows
    const uint32_t row_width_bits = dib_header->width * dib_header->bits_per_pixel;
    const uint32_t row_padding_bits = (32 - (row_width_bits % 32)) % 32;
    const uint32_t padded_row_width_bytes = (row_width_bits + row_padding_bits) / 8;

    dib_header_metadata.padded_row_width = padded_row_width_bytes;
    // Alpha
    dib_header_metadata.has_alpha_channel = dib_header->masks_rgba.alpha_mask != 0;

    return dib_header_metadata;
  }
}
