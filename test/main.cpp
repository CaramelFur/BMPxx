#include "bmpxx.hpp"

#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <algorithm> // for std::copy
#include <iomanip>

int main(int ac, char **av)
{
  // Read first argument to string
  if (ac < 3)
  {
    std::cerr << "Usage: " << av[0] << " <input> <output> [width] [height] [channels]" << std::endl;
    return 1;
  }

  std::string input_filename = av[1];
  std::string output_filename = av[2];

  // Read file into vector<byte>
  std::ifstream file(input_filename, std::ios::binary);
  if (!file)
  {
    std::cerr << "Could not open file " << input_filename << std::endl;
    return 1;
  }

  std::vector<uint8_t> inputImage((std::istreambuf_iterator<char>(file)),
                                  (std::istreambuf_iterator<char>()));

  // if input ends with .bmp
  if (input_filename.size() > 4 && input_filename.substr(input_filename.size() - 4) == ".bmp")
  {
    auto result = bmpxx::bmp::decode(inputImage);

    std::vector<uint8_t> outputImage = result.first;
    bmpxx::BmpDesc description = result.second;

    // Print description
    std::cout << "Successfully decoded " << input_filename << " to " << output_filename << std::endl;
    std::cout << "Width: " << description.width << std::endl;
    std::cout << "Height: " << description.height << std::endl;
    std::cout << "Channels: " << (int)description.channels << std::endl;

    std::ofstream outfile(output_filename, std::ios::binary);
    outfile.write(reinterpret_cast<char *>(outputImage.data()), outputImage.size());
    outfile.close();
  }
  else if (input_filename.size() > 4 && input_filename.substr(input_filename.size() - 4) == ".ico")
  {
    auto result = bmpxx::bmp::decode_icon(inputImage);

    std::vector<uint8_t> outputImage = result.first;
    bmpxx::BmpDesc description = result.second;

    // Print description
    std::cout << "Successfully decoded " << input_filename << " to " << output_filename << std::endl;
    std::cout << "Width: " << description.width << std::endl;
    std::cout << "Height: " << description.height << std::endl;
    std::cout << "Channels: " << (int)description.channels << std::endl;

    std::ofstream outfile(output_filename, std::ios::binary);
    outfile.write(reinterpret_cast<char *>(outputImage.data()), outputImage.size());
    outfile.close();
  }
  else if (output_filename.size() > 4 && output_filename.substr(output_filename.size() - 4) == ".bmp")
  {
    if (ac < 6)
    {
      std::cerr << "Usage: " << av[0] << " <input> <output> [width] [height] [channels]" << std::endl;
      return 1;
    }

    bmpxx::BmpDesc description(std::stoi(av[3]), std::stoi(av[4]), (uint8_t)std::stoi(av[5]));

    std::vector<uint8_t> outputImage = bmpxx::bmp::encode(inputImage, description);

    std::cout << "Successfully encoded " << input_filename << " to " << output_filename << std::endl;

    std::ofstream outfile(output_filename, std::ios::binary);
    outfile.write(reinterpret_cast<char *>(outputImage.data()), outputImage.size());
    outfile.close();
  }

  return 0;
}
