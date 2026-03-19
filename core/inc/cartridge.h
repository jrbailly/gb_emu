#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include "ram.h"
#include <array>
#include <cstdint>
#include <vector>

static constexpr size_t block_size = 0x4000;
static constexpr size_t rom_ram_address = 0xA000;
static constexpr size_t rom_ram_size = 0x2000;

class Cartridge
{
  public:
    enum Register
    {
        CGB_FLAG = 0x143,          ///< CGB compatibility flag
        SGB_FLAG = 0x146,          ///< SGB feature flag
        CARTRIDGE_TYPE = 0x147,    ///< Cartridge hardware type
        MBC_ROM_BANK = 0x2000,     ///< MBC1 ROM bank number select
        MBC_RAM_BANK = 0x4000,     ///< MBC1 RAM bank number / upper ROM bank bits
        MBC_BANKING_MODE = 0x6000, ///< MBC1 banking mode select
    };
    enum CgbFlag
    {
        DMG_ONLY = 0x00, ///< DMG cartridge, no CGB support
        DMG_CGB = 0x80,  ///< Backward-compatible with both DMG and CGB
        CGB_ONLY = 0xC0  ///< CGB exclusive, not supported
    };
    enum SgbFlag
    {
        NO_SGB = 0x00, ///< No SGB functions
        SGB = 0x03     ///< SGB functions present, not supported
    };
    enum CartridgeType
    {
        ROM_ONLY = 0x00,                       ///< No MBC — supported
        MBC1 = 0x01,                           ///< MBC1 — supported
        MBC1_RAM = 0x02,                       ///< MBC1 + RAM
        MBC1_RAM_BATTERY = 0x03,               ///< MBC1 + RAM + Battery
        MBC2 = 0x05,                           ///< MBC2
        MBC2_BATTERY = 0x06,                   ///< MBC2 + Battery
        ROM_RAM = 0x08,                        ///< ROM + RAM
        ROM_RAM_BATTERY = 0x09,                ///< ROM + RAM + Battery
        MMM01 = 0x0B,                          ///< MMM01
        MMM01_RAM = 0x0C,                      ///< MMM01 + RAM
        MMM01_RAM_BATTERY = 0x0D,              ///< MMM01 + RAM + Battery
        MBC3_TIMER_BATTERY = 0x0F,             ///< MBC3 + Timer + Battery
        MBC3_TIMER_RAM_BATTERY = 0x10,         ///< MBC3 + Timer + RAM + Battery
        MBC3 = 0x11,                           ///< MBC3
        MBC3_RAM = 0x12,                       ///< MBC3 + RAM
        MBC3_RAM_BATTERY = 0x13,               ///< MBC3 + RAM + Battery
        MBC5 = 0x19,                           ///< MBC5
        MBC5_RAM = 0x1A,                       ///< MBC5 + RAM
        MBC5_RAM_BATTERY = 0x1B,               ///< MBC5 + RAM + Battery
        MBC5_RUMBLE = 0x1C,                    ///< MBC5 + Rumble
        MBC5_RUMBLE_RAM = 0x1D,                ///< MBC5 + Rumble + RAM
        MBC5_RUMBLE_RAM_BATTERY = 0x1E,        ///< MBC5 + Rumble + RAM + Battery
        MBC6 = 0x20,                           ///< MBC6
        MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22, ///< MBC7 + Sensor + Rumble + RAM + Battery
        POCKET_CAMERA = 0xFC,                  ///< Pocket Camera
        BANDAI_TAMA5 = 0xFD,                   ///< Bandai TAMA5
        HUC3 = 0xFE,                           ///< HuC3
        HUC1_RAM_BATTERY = 0xFF                ///< HuC1 + RAM + Battery
    };
    /**
     * @brief Construct a new Cartridge object.
     */
    Cartridge();

    /**
     * @brief Register MBC write callbacks on the bus.
     *
     * @param ram Instance of RamBus object
     */
    auto init(RamBus &ram) -> void;

    /**
     * @brief Write block 0 and block 1 into RAM.
     *
     * @param ram Instance of RamBus object
     */
    auto load_rom(RamBus &ram) -> void;

    /**
     * @brief Read a ROM file, validate DMG-only header fields, and store the
     *        decoded CGB flag, SGB flag and cartridge type.
     *
     * @param filename Path to the .gb ROM file
     * @throws std::runtime_error if the file cannot be opened, is empty, or
     *         contains an unsupported CGB flag, SGB flag or cartridge type
     */
    auto read_rom(const std::string_view filename) -> void;

  private:
    auto validate_header() -> void;

  private:
    int _mode;
    size_t _bank;
    size_t _upper_bank;
    size_t _bank_mask;
    CgbFlag _cgb_flag;
    SgbFlag _sgb_flag;
    CartridgeType _cartridge_type;
    std::vector<std::array<unsigned char, block_size>> _rom;
};

#endif
