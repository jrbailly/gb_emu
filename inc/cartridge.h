#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include <array>
#include <string>
#include <vector>
static constexpr size_t BLOCK_SIZE = 0x4000;
static constexpr size_t ROM_RAM_ADDRESS = 0xA000;
static constexpr size_t ROM_RAM_SIZE = 0x2000;

class Cartridge
{
  public:
    Cartridge(const std::string_view filename);
    auto GetBank(size_t bank) const -> const std::array<unsigned char, BLOCK_SIZE> &;
    auto GetBankCount() const -> size_t;

  private:
    auto ReadROM(const std::string_view filename) -> void;

  private:
    std::vector<std::array<unsigned char, BLOCK_SIZE>> mRom;
};

#endif