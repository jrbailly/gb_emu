#ifndef _RAM_H_
#define _RAM_H_
#include "cartridge.h"
#include <array>
#include <cstdint>
#include <memory>
static constexpr std::size_t RAM_SIZE = 0x10000;

class MBC1
{
    enum
    {
        ROM_BANK_CHANGE = 0x2000,
        UPPER_ROM_BANK_CHANGE = 0x4000,
        ROM_RAM_CHANGE = 0x6000,
    };

  public:
    MBC1(std::string_view romfile);
    inline unsigned char operator[](std::size_t i) const
    {
        return mRam[i];
    }
    inline void write(uint16_t address, uint8_t value)
    {
        if ((address & 0xE000) == ROM_BANK_CHANGE)
        {
            std::copy(mCartridge->GetBank(value).begin(), mCartridge->GetBank(value).end(), mRam.begin() + 0x4000);
        }
    }
    void init();
    const unsigned char *data() const
    {
        return mRam.data();
    }

  private:
    std::array<unsigned char, RAM_SIZE> mRam;
    std::unique_ptr<Cartridge> mCartridge;
};
#endif