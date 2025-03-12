#ifndef _RAM_H_
#define _RAM_H_
#include "cartridge.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>

static constexpr std::size_t RAM_SIZE = 0x10000;

class MBC1
{
  public:
    MBC1(std::string_view romfile);
    inline unsigned char operator[](std::size_t i) const
    {
        return mRam[i];
    }
    inline void write(uint16_t address, uint8_t value)
    {
        if (address >= 0x2000 && address <= 0x3FFF) // ROM bank change
        {
            std::copy(mCartridge->GetBank(mUpperBank + value).begin(), mCartridge->GetBank(mUpperBank + value).end(),
                      mRam.begin() + 0x4000);
        }
        else if (address >= 0x4000 && address <= 0x5FFF) // Upper ROM bank change
        {
            if (mMode == 0)
                mUpperBank = value << 5;
            // else
        }
        else if (address >= 0x6000 && address <= 0x7FFF) // ROM / RAM change
        {
            mMode = value;
        }
        else if (address == 0xFF46)
        {
            uint16_t start_address = value << 8;
            uint16_t end_address = start_address + 160;
            uint16_t dst_address = 0xFE00;
            std::copy(mRam.begin() + start_address, mRam.begin() + end_address, mRam.begin() + dst_address);
        }
        else if (address == 0xFF02)
        {
            if ((value & 0x80) == 0x80)
            {
                std::cout << mRam[0xFF01];
                mRam[0xFF01] &= 0xEF;
            }
        }
        mRam[address] = value;
        mWriteAddress = address;
    }
    void init();
    const unsigned char *data() const
    {
        return mRam.data();
    }
    const uint16_t lastWrite() const
    {
        return mWriteAddress;
    }
    void resetLastWrite()
    {
        mWriteAddress = 0;
    }

  private:
    uint8_t mMode;
    uint16_t mUpperBank;
    uint16_t mWriteAddress;
    std::array<unsigned char, RAM_SIZE> mRam;
    std::unique_ptr<Cartridge> mCartridge;
};
#endif