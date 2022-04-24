# bmpxx

A c++ library for reading and writing bitmap / BMP files

## Why?

I needed a bmp library for another project and I couldn't find any that supported enough parts of the bmp specification. So I just wrote my own, and hopefully it will be of use to some other people as well.

If it helped you, don't forget to leave the repo a star.

## Support

### Decoding

- All NT (Windows NT) headers
- 1, 2, 4, 8 bit rgb palette images
- 16, 24, 32 bit rgb/rgba images
- Alpha channel
- Any sane combination of pixel masks
- Correct bit mapping using floats
- `BI_RGB`, `BI_BITFIELDS`, `BI_ALPHABITFIELDS` compression
- 8 bit color depth (so no 10 bit)

### Encoding

- 56 byte NT header
- 24 and 32 bit rgb/rgba images
- Alpha channel
- `BI_RGB`, `BI_BITFIELDS` compression
- 8 bit color depth

## Api

### Functions

```cpp
namespace bmpxx {

// Decodes a bmp file into either an RGB or RGBA pixel array
// Returns the width, height and channel count of the image
std::pair<std::vector<uint8_t>, BmpDesc> bmp::decode(std::vector<uint8_t> inputImage);

// Encodes an RGB or RGBA pixel array into a bmp file
// Returns the encoded bmp file
std::vector<uint8_t> bmp::encode(std::vector<uint8_t> input, BmpDesc desc);

}
```

### Structs

```cpp
struct BmpDesc
{
  int32_t width;
  int32_t height;
  uint8_t channels;
}
```

## Example

For an example see the [test program](https://github.com/rubikscraft/bmpxx/blob/master/test/main.cpp).

This program can convert between bmp and the raw rgb/rgba pixel arrays.

## Helping

This library also does not support all the bmp features, so if you are missing something, feel free to submit a pull request.

## License

This code is licensed under the GNU General Public License v3.0, so any changes or additions to this code will have to be made public under the same license.
