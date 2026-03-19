#include "cartridge.h"
#include "ram.h"
#include <bit>
#include <format>
#include <fstream>
#include <stdexcept>

/**
 * @brief Construct a new Cartridge object
 *
 */
Cartridge::Cartridge()
    : _mode(0), _bank(0), _upper_bank(0), _bank_mask(0), _cgb_flag(DMG_ONLY), _sgb_flag(NO_SGB),
      _cartridge_type(ROM_ONLY)
{
}

/**
 * @brief Write callback
 *
 * @param ram Instance of RamBus object
 */
auto Cartridge::init(RamBus &ram) -> void
{
    ram.register_callback_range(MBC_ROM_BANK, block_size, [this](RamBus &ram, int, unsigned char val) {
        if (val == 0)
            val = 1;
        _bank = val & _bank_mask;
        ram.write_range(_rom[_upper_bank + _bank].data(), block_size, 0x4000);
    });
    ram.register_callback_range(MBC_RAM_BANK, 0x2000, [](RamBus &, int, unsigned char) {});
    ram.register_callback_range(MBC_BANKING_MODE, 0x2000, [this](RamBus &, int, unsigned char val) { _mode = val; });
}

/**
 * @brief Read a ROM file and load it as 16 KB blocks into the internal ROM vector.
 *
 * @param filename Path to the .gb ROM file.
 * @throws std::runtime_error if the file cannot be opened or is empty.
 */
auto Cartridge::read_rom(const std::string_view filename) -> void
{
    std::ifstream input(filename.data(), std::ios::binary);
    std::streamsize bytesRead;

    if (!input)
        throw std::runtime_error(std::format("Failed to open ROM file: {}", filename));
    do
    {
        std::array<unsigned char, block_size> block;

        input.read(reinterpret_cast<char *>(block.data()), block_size);
        bytesRead = input.gcount();
        if (bytesRead == block_size)
            _rom.push_back(block);
    } while (bytesRead == block_size);

    if (_rom.empty())
        throw std::runtime_error(std::format("ROM file is empty: {}", filename));

    validate_header();
}

/**
 * @brief Validate DMG-only header fields and store the decoded values.
 *
 * @throws std::runtime_error if the cartridge contains an unsupported CGB flag,
 *         SGB flag or cartridge type
 */
auto Cartridge::validate_header() -> void
{
    auto cgb_byte = _rom[0][Register::CGB_FLAG];
    auto sgb_byte = _rom[0][Register::SGB_FLAG];
    auto type_byte = _rom[0][Register::CARTRIDGE_TYPE];

    if (cgb_byte != DMG_ONLY && cgb_byte != DMG_CGB)
        throw std::runtime_error(
            std::format("Unsupported CGB flag: 0x{:02X} (CGB-only cartridges are not supported)", cgb_byte));

    if (type_byte != ROM_ONLY && type_byte != MBC1 && type_byte != MBC1_RAM && type_byte != MBC1_RAM_BATTERY)
        throw std::runtime_error(std::format(
            "Unsupported cartridge type: 0x{:02X} (only ROM-only and MBC1 cartridges are supported)", type_byte));

    _cgb_flag = static_cast<CgbFlag>(cgb_byte);
    _sgb_flag = static_cast<SgbFlag>(sgb_byte);
    _cartridge_type = static_cast<CartridgeType>(type_byte);

    _bank_mask = std::bit_ceil(_rom.size()) - 1;
}

/**
 * @brief Write block 0 and block 1 into RAM
 *
 * @param ram Instance of RamBus object
 */
auto Cartridge::load_rom(RamBus &ram) -> void
{
    if (_rom.size() > 0)
        ram.write_range(_rom[0].data(), block_size, 0x0);
    if (_rom.size() > 1)
        ram.write_range(_rom[1].data(), block_size, block_size);
}
