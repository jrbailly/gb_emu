#include "cartridge.h"
#include "ram.h"
#include <format>
#include <fstream>
#include <stdexcept>
/**
 * @brief Construct a new Cartridge object
 *
 */
Cartridge::Cartridge() : _mode(0), _bank(0), _upper_bank(0)
{
}

/**
 * @brief Write callback
 *
 * @param ram Instance of RamBus object
 */
auto Cartridge::init(RamBus &ram) -> void
{
    ram.register_callback_range(0x2000, 0x4000, [this](RamBus &ram, int addr, unsigned char val) {
        if (_upper_bank + val > _rom.size())
            throw std::runtime_error(std::format("Cartridge::init invalid bank : ") +
                                     std::to_string(_upper_bank + val));
        if (val == 0)
            val = 1;
        _bank = val;
        ram.write_range(_rom[_upper_bank + _bank].begin(), BLOCK_SIZE, 0x4000);
    });
    ram.register_callback_range(0x4000, 0x2000, [this](RamBus &ram, int addr, unsigned char val) {
        // if (_mode == 0)
        //     _upper_bank = (val & 0x3) << 5;
    });
    ram.register_callback_range(0x6000, 0x2000, [this](RamBus &ram, int addr, unsigned char val) { _mode = val; });
}

/**
 * @brief Read ROM from ".gb" file
 *
 * @param filename ROM filename
 */
auto Cartridge::read_rom(const std::string_view filename) -> void
{
    std::ifstream input(filename.data(), std::ios::binary);
    std::streamsize bytesRead;

    if (!input)
        throw std::runtime_error(std::format("Failed to open ROM file: ") + filename.data());
    do
    {
        std::array<unsigned char, BLOCK_SIZE> block;

        input.read(reinterpret_cast<char *>(block.data()), BLOCK_SIZE);
        bytesRead = input.gcount();
        if (bytesRead == BLOCK_SIZE)
            _rom.push_back(block);
    } while (bytesRead == BLOCK_SIZE);
}

/**
 * @brief Write block 0 and block 1 into RAM
 *
 * @param ram Instance of RamBus object
 */
auto Cartridge::load_rom(RamBus &ram) -> void
{
    if (_rom.size() > 0)
        ram.write_range(_rom[0].begin(), BLOCK_SIZE, 0x0);
    if (_rom.size() > 1)
        ram.write_range(_rom[1].begin(), BLOCK_SIZE, BLOCK_SIZE);
}