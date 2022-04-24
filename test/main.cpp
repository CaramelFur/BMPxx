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
  if (ac < 2)
  {
    std::cerr << "Usage: " << av[0] << " <filename>" << std::endl;
    return 1;
  }

  std::string filename = av[1];

  // Read file into vector<byte>
  std::ifstream file(filename, std::ios::binary);
  if (!file)
  {
    std::cerr << "Could not open file " << filename << std::endl;
    return 1;
  }

  std::vector<uint8_t> inputImage((std::istreambuf_iterator<char>(file)),
                                  (std::istreambuf_iterator<char>()));

  auto result = bmpxx::decode(inputImage);

  std::vector<uint8_t> outputImage = result.first;
  bmpxx::bmp_desc description = result.second;

  auto result2 = bmpxx::encode(outputImage, description);

  // // Print outputImage bytes
  // for (int i = 0; i < outputImage.size(); i++)
  // {
  //   // Print byte in hex padded with 0
  //   std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)outputImage[i] << " ";
  //   if (i % (description.width * description.channels) == (description.width * description.channels) - 1)
  //     std::cout << std::endl;
  // }
  // std::cout << std::endl;

  // Save outputimage
  std::ofstream outfile("test.bmp", std::ios::binary);
  outfile.write(reinterpret_cast<char *>(result2.data()), result2.size());
  outfile.close();

  return 0;
}
