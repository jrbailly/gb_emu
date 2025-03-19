#include "ram.h"
#include <fstream>

MBC1::MBC1(std::string_view romfile)
    : mUpperBank(0), mFilename(romfile), mCartridge(std::make_unique<Cartridge>(romfile))
{
}

void MBC1::init()
{
    mRam.fill(0);
    std::copy(mCartridge->GetBank(0).begin(), mCartridge->GetBank(0).end(), mRam.begin());
    std::copy(mCartridge->GetBank(1).begin(), mCartridge->GetBank(1).end(), mRam.begin() + 0x4000);
}

auto MBC1::LoadSaveRAM() -> void
{
    std::string filename = mFilename + ".ram";
    std::ifstream input(filename.data(), std::ios::binary);
    std::streamsize bytesRead;

    if (input)
    {
        std::array<unsigned char, ROM_RAM_SIZE> block;

        input.read(reinterpret_cast<char *>(block.data()), ROM_RAM_SIZE);
        bytesRead = input.gcount();
        if (bytesRead == ROM_RAM_SIZE)
            std::copy(block.begin(), block.end(), mRam.begin() + ROM_RAM_ADDRESS);
    };
}

auto MBC1::SaveRAM() -> void
{
    std::string filename = mFilename + ".ram";
    std::ofstream output(filename.data(), std::ios::binary);
    std::streamsize bytesRead;

    if (output)
        output.write(reinterpret_cast<char *>(&mRam[ROM_RAM_ADDRESS]), ROM_RAM_SIZE);
}