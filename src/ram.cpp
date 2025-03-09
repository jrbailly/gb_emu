#include "ram.h"

MBC1::MBC1(std::string_view romfile) : mCartridge(std::make_unique<Cartridge>(romfile))
{
}

void MBC1::init()
{
    mRam.fill(0);
    std::copy(mCartridge->GetBank(0).begin(), mCartridge->GetBank(0).end(), mRam.begin());
    std::copy(mCartridge->GetBank(1).begin(), mCartridge->GetBank(1).end(), mRam.begin() + 0x4000);
}
