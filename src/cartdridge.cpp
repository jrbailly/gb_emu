#include "cartridge.h"
#include "ram.h"
#include <fstream>
#include <stdexcept>

Cartridge::Cartridge() : mUpperBank(0), mMode(0)
{
}

auto Cartridge::init(MBC1 &ram) -> void
{
    for (int i = 0x2000; i < 0x4000; ++i)
        ram.RegisterCallback(i, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
            if (val == 0)
                val = 1;
            std::copy(mRom[mUpperBank + val].begin(), mRom[mUpperBank + val].end(), ram.begin() + 0x4000);
        });
    for (int i = 0x4000; i < 0x6000; ++i)
        ram.RegisterCallback(i, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
            if (mMode == 0)
                mUpperBank = val << 5;
        });
    for (int i = 0x6000; i < 0x8000; ++i)
        ram.RegisterCallback(i, [this](MBC1 &ram, uint16_t addr, uint8_t val) { mMode = val; });
}

auto Cartridge::GetBank(size_t bank) const -> const std::array<unsigned char, BLOCK_SIZE> &
{
    if (bank >= mRom.size())
        throw std::runtime_error("Invalid bank " + std::to_string(bank));
    return (mRom[bank]);
}

inline auto Cartridge::GetBankCount() const -> size_t
{
    return (mRom.size());
}

auto Cartridge::ReadROM(const std::string_view filename) -> void
{
    std::ifstream input(filename.data(), std::ios::binary);
    std::streamsize bytesRead;

    if (!input)
        throw std::runtime_error(std::string("Failed to open ROM file: ") + filename.data());
    do
    {
        std::array<unsigned char, BLOCK_SIZE> block;

        input.read(reinterpret_cast<char *>(block.data()), BLOCK_SIZE);
        bytesRead = input.gcount();
        if (bytesRead == BLOCK_SIZE)
            mRom.push_back(block);
    } while (bytesRead == BLOCK_SIZE);
}

auto Cartridge::LoadROM(MBC1 &ram) -> void
{
    std::copy(mRom[0].begin(), mRom[0].end(), ram.begin());
    std::copy(mRom[1].begin(), mRom[1].end(), ram.begin() + 0x4000);
}