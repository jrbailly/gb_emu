#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include "ram.h"
#include <array>
#include <vector>

static constexpr size_t BLOCK_SIZE = 0x4000;
static constexpr size_t ROM_RAM_ADDRESS = 0xA000;
static constexpr size_t ROM_RAM_SIZE = 0x2000;

class Cartridge
{
  public:
    Cartridge();
    auto init(RamBus &ram) -> void;
    auto load_rom(RamBus &ram) -> void;
    auto read_rom(const std::string_view filename) -> void;

  private:
    int _mode;
    int _upper_bank;
    std::vector<std::array<unsigned char, BLOCK_SIZE>> _rom;
};

#endif