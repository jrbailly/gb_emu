#include "ram.h"

MBC1::MBC1(std::string_view romfile) : mCartridge(std::make_unique<Cartridge>(romfile))
{
}

void MBC1::init()
{
    mRam.fill(0);
    std::copy(mCartridge->GetBank(0).begin(), mCartridge->GetBank(0).end(), mRam.begin());
}
