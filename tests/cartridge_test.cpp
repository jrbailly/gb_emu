#include "cartridge.h"
#include "ram.h"
#include <array>
#include <fstream>
#include <gtest/gtest.h>

// ROM memory layout:
//   0x0000–0x3FFF : block 0 (fixed bank, loaded by load_rom)
//   0x4000–0x7FFF : switchable bank (block 1 by default, changed via MBC1)
//
// MBC1 write ranges (active after init):
//   0x2000–0x3FFF : ROM bank select — 5-bit value, 0 is remapped to 1
//   0x4000–0x5FFF : RAM bank / upper ROM bank (empty callback — not used)
//   0x6000–0x7FFF : banking mode select
//
// Accepted header values:
//   CGB flag     : 0x00 (DMG_ONLY) or 0x80 (DMG_CGB)
//   Cartridge    : ROM_ONLY (0x00), MBC1 (0x01), MBC1_RAM (0x02), MBC1_RAM_BATTERY (0x03)
//
// Block fill pattern used in tests:
//   block 0 → 0xAA, block 1 → 0xBB, block 2 → 0xCC, block 3 → 0xDD
// Header bytes inside block 0 are patched after filling, so the fill is only visible
// at non-header addresses (0x0000, 0x4000, etc.).

static constexpr const char *ROM_PATH = "/tmp/gb_emu_test_cartridge.gb";

class CartridgeTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ram.clear();
    }

    /**
     * @brief Returns the expected fill byte for a given block index.
     * @details block 0 → 0xAA, block 1 → 0xBB, block 2 → 0xCC, block 3 → 0xDD.
     */
    static constexpr uint8_t bank_sig(int index)
    {
        return static_cast<uint8_t>(0xAA + index * 0x11);
    }

    /**
     * @brief Writes a synthetised ROM file to disk.
     *
     * Each block is filled with bank_sig(i). Block 0 additionally has its header bytes
     * patched: CGB flag at 0x143, SGB flag at 0x146, cartridge type at 0x147.
     *
     * @param path            Destination file path.
     * @param num_banks       Number of 16 KB blocks to write.
     * @param cgb_flag        CGB compatibility flag for the header (default DMG_ONLY).
     * @param cartridge_type  Cartridge type byte for the header (default MBC1).
     */
    static void write_temp_rom(const std::string &path,
                                int num_banks,
                                uint8_t cgb_flag = Cartridge::DMG_ONLY,
                                uint8_t cartridge_type = Cartridge::MBC1)
    {
        std::ofstream f(path, std::ios::binary);
        for (int i = 0; i < num_banks; ++i)
        {
            std::array<unsigned char, block_size> block;
            block.fill(bank_sig(i));
            if (i == 0)
            {
                block[Cartridge::Register::CGB_FLAG] = cgb_flag;
                block[Cartridge::Register::SGB_FLAG] = Cartridge::NO_SGB;
                block[Cartridge::Register::CARTRIDGE_TYPE] = cartridge_type;
            }
            f.write(reinterpret_cast<const char *>(block.data()), block_size);
        }
    }

    /**
     * @brief Writes a 4-bank MBC1 ROM and runs the full init sequence.
     *
     * After this call:
     *   ram[0x0000] = 0xAA (block 0 fill, header bytes overwrite specific offsets only)
     *   ram[0x4000] = 0xBB (block 1 fill, initial switchable bank)
     */
    void init_4bank_mbc1()
    {
        write_temp_rom(ROM_PATH, 4);
        cartridge.read_rom(ROM_PATH);
        cartridge.init(ram);
        cartridge.load_rom(ram);
    }

    RamBus ram{true};
    Cartridge cartridge{};
};

// ---------------------------------------------------------------------------
// File reading
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that read_rom throws when the file does not exist.
 */
TEST_F(CartridgeTest, ReadRom_ThrowsOnFileNotFound)
{
    EXPECT_THROW(cartridge.read_rom("/nonexistent/path/rom.gb"), std::runtime_error);
}

/**
 * @brief Verifies that read_rom throws when the file is smaller than one full block (16 KB).
 * @details read_rom only pushes a block when exactly block_size bytes were read. A file of
 *          100 bytes leaves _rom empty, which triggers the "ROM file is empty" exception.
 */
TEST_F(CartridgeTest, ReadRom_ThrowsOnFileSmallerThanOneBlock)
{
    {
        std::ofstream f(ROM_PATH, std::ios::binary);
        std::array<char, 100> tiny{};
        f.write(tiny.data(), static_cast<std::streamsize>(tiny.size()));
    }
    EXPECT_THROW(cartridge.read_rom(ROM_PATH), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Header validation — CGB flag
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that CGB flag 0x00 (DMG_ONLY) is accepted.
 */
TEST_F(CartridgeTest, ReadRom_AcceptsCGBFlag_DMG_ONLY)
{
    write_temp_rom(ROM_PATH, 1, Cartridge::DMG_ONLY, Cartridge::ROM_ONLY);
    EXPECT_NO_THROW(cartridge.read_rom(ROM_PATH));
}

/**
 * @brief Verifies that CGB flag 0x80 (DMG_CGB, backward-compatible) is accepted.
 */
TEST_F(CartridgeTest, ReadRom_AcceptsCGBFlag_DMG_CGB)
{
    write_temp_rom(ROM_PATH, 1, Cartridge::DMG_CGB, Cartridge::ROM_ONLY);
    EXPECT_NO_THROW(cartridge.read_rom(ROM_PATH));
}

/**
 * @brief Verifies that CGB flag 0xC0 (CGB_ONLY) is rejected.
 * @details CGB-only cartridges are not supported by this DMG emulator.
 */
TEST_F(CartridgeTest, ReadRom_ThrowsOnCGBFlag_CGB_ONLY)
{
    write_temp_rom(ROM_PATH, 1, Cartridge::CGB_ONLY, Cartridge::ROM_ONLY);
    EXPECT_THROW(cartridge.read_rom(ROM_PATH), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Header validation — cartridge type
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that ROM_ONLY (0x00) cartridges are accepted.
 */
TEST_F(CartridgeTest, ReadRom_AcceptsCartridgeType_ROM_ONLY)
{
    write_temp_rom(ROM_PATH, 1, Cartridge::DMG_ONLY, Cartridge::ROM_ONLY);
    EXPECT_NO_THROW(cartridge.read_rom(ROM_PATH));
}

/**
 * @brief Verifies that MBC1 (0x01) cartridges are accepted.
 */
TEST_F(CartridgeTest, ReadRom_AcceptsCartridgeType_MBC1)
{
    write_temp_rom(ROM_PATH, 2, Cartridge::DMG_ONLY, Cartridge::MBC1);
    EXPECT_NO_THROW(cartridge.read_rom(ROM_PATH));
}

/**
 * @brief Verifies that MBC1+RAM (0x02) cartridges are accepted.
 */
TEST_F(CartridgeTest, ReadRom_AcceptsCartridgeType_MBC1_RAM)
{
    write_temp_rom(ROM_PATH, 2, Cartridge::DMG_ONLY, Cartridge::MBC1_RAM);
    EXPECT_NO_THROW(cartridge.read_rom(ROM_PATH));
}

/**
 * @brief Verifies that MBC1+RAM+Battery (0x03) cartridges are accepted.
 */
TEST_F(CartridgeTest, ReadRom_AcceptsCartridgeType_MBC1_RAM_BATTERY)
{
    write_temp_rom(ROM_PATH, 2, Cartridge::DMG_ONLY, Cartridge::MBC1_RAM_BATTERY);
    EXPECT_NO_THROW(cartridge.read_rom(ROM_PATH));
}

/**
 * @brief Verifies that unsupported cartridge types are rejected.
 * @details MBC2 (0x05) is not supported — read_rom must throw std::runtime_error.
 */
TEST_F(CartridgeTest, ReadRom_ThrowsOnUnsupportedCartridgeType_MBC2)
{
    write_temp_rom(ROM_PATH, 2, Cartridge::DMG_ONLY, static_cast<uint8_t>(Cartridge::MBC2));
    EXPECT_THROW(cartridge.read_rom(ROM_PATH), std::runtime_error);
}

// ---------------------------------------------------------------------------
// ROM loading — load_rom maps blocks into RAM
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that block 0 is mapped to address 0x0000 after load_rom.
 * @details Byte 0x0000 lies well below the header offsets (0x143+), so it holds the
 *          raw fill pattern 0xAA.
 */
TEST_F(CartridgeTest, LoadRom_Block0_MappedAt0x0000)
{
    write_temp_rom(ROM_PATH, 2);
    cartridge.read_rom(ROM_PATH);
    cartridge.init(ram);
    cartridge.load_rom(ram);

    EXPECT_EQ(ram[0x0000], bank_sig(0));
}

/**
 * @brief Verifies that block 1 is mapped to address 0x4000 after load_rom.
 */
TEST_F(CartridgeTest, LoadRom_Block1_MappedAt0x4000)
{
    write_temp_rom(ROM_PATH, 2);
    cartridge.read_rom(ROM_PATH);
    cartridge.init(ram);
    cartridge.load_rom(ram);

    EXPECT_EQ(ram[0x4000], bank_sig(1));
}

/**
 * @brief Verifies that load_rom does not write to 0x4000 when there is only one block.
 * @details A sentinel value is planted at 0x4000 before load_rom. After loading a single-block
 *          ROM_ONLY cartridge, the sentinel must be intact.
 */
TEST_F(CartridgeTest, LoadRom_SingleBlock_DoesNotOverwrite0x4000)
{
    ram.write_register(0x4000, 0x5A);

    write_temp_rom(ROM_PATH, 1, Cartridge::DMG_ONLY, Cartridge::ROM_ONLY);
    cartridge.read_rom(ROM_PATH);
    cartridge.init(ram);
    cartridge.load_rom(ram);

    EXPECT_EQ(ram[0x4000], 0x5A);
}

// ---------------------------------------------------------------------------
// MBC1 bank switching
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that writing a bank number to MBC_ROM_BANK maps the correct block to 0x4000.
 * @details Starting from the initial bank 1, switches to bank 2 then bank 3 and checks that
 *          the fill pattern at 0x4000 matches the newly selected block.
 */
TEST_F(CartridgeTest, MBC1_BankSwitch_SelectsCorrectBlock)
{
    init_4bank_mbc1();
    ASSERT_EQ(ram[0x4000], bank_sig(1)); // initial state: bank 1

    ram.write(Cartridge::Register::MBC_ROM_BANK, 2);
    EXPECT_EQ(ram[0x4000], bank_sig(2));

    ram.write(Cartridge::Register::MBC_ROM_BANK, 3);
    EXPECT_EQ(ram[0x4000], bank_sig(3));
}

/**
 * @brief Verifies the hardware quirk: writing 0 to MBC_ROM_BANK selects bank 1 instead.
 * @details The MBC1 hardware remaps bank index 0 to 1 to prevent bank 0 from being mapped
 *          twice (it is always visible at 0x0000). The implementation mirrors this: if the
 *          written value is 0, it is replaced by 1 before masking.
 */
TEST_F(CartridgeTest, MBC1_BankSwitch_WritingZeroSelectsBank1)
{
    init_4bank_mbc1();

    // Switch away from bank 1 first so the correction is observable.
    ram.write(Cartridge::Register::MBC_ROM_BANK, 2);
    ASSERT_EQ(ram[0x4000], bank_sig(2));

    ram.write(Cartridge::Register::MBC_ROM_BANK, 0);
    EXPECT_EQ(ram[0x4000], bank_sig(1));
}

/**
 * @brief Verifies that the bank mask limits the bank index to the ROM size.
 * @details With a 4-bank ROM, _bank_mask = bit_ceil(4) − 1 = 3 (0b11).
 *          Writing 7 (0b111): 7 & 3 = 3 → selects block 3.
 *          The mask prevents an out-of-bounds access into the ROM vector.
 */
TEST_F(CartridgeTest, MBC1_BankSwitch_BankMaskApplied)
{
    init_4bank_mbc1();

    ram.write(Cartridge::Register::MBC_ROM_BANK, 7); // 7 & 3 = 3
    EXPECT_EQ(ram[0x4000], bank_sig(3));
}

/**
 * @brief Verifies that masking a value to 0 does not trigger the 0→1 correction.
 * @details The zero correction is applied to the raw written value, not to the masked result.
 *          With a 4-bank ROM and mask 0b11, writing 4 (0b100): val=4 ≠ 0 so no correction,
 *          then _bank = 4 & 3 = 0 → block 0 is mapped to 0x4000.
 */
TEST_F(CartridgeTest, MBC1_BankSwitch_MaskToZeroNoCorrection)
{
    init_4bank_mbc1();

    ram.write(Cartridge::Register::MBC_ROM_BANK, 4); // 4 & 3 = 0, no 0→1 correction
    EXPECT_EQ(ram[0x4000], bank_sig(0));             // block 0 data visible at 0x4000
}

/**
 * @brief Verifies that writing to any address in the ROM bank range triggers the switch.
 * @details The callback covers 0x2000–0x3FFF (0x2000 bytes, before the RAM bank range
 *          overrides 0x4000–0x5FFF). Writing to 0x3FFF must produce the same effect as
 *          writing to 0x2000.
 */
TEST_F(CartridgeTest, MBC1_BankSwitch_AnyAddressInRangeTriggersSwitch)
{
    init_4bank_mbc1();

    ram.write(0x3FFF, 2); // last address in the ROM bank select range
    EXPECT_EQ(ram[0x4000], bank_sig(2));
}
