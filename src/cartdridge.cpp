#include "cartridge.h"
#include "ram.h"
#include <fstream>
#include <stdexcept>

Cartridge::Cartridge(const std::string_view filename)
{
    ReadROM(filename);
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
