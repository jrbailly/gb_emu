#ifndef _RAM_H_
#define _RAM_H_
#include "cartridge.h"
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

static constexpr std::size_t RAM_SIZE = 0x10000;
typedef std::function<void(MBC1 &ram, uint16_t, uint8_t)> RamCallback;

class MBC1
{
  public:
    MBC1(std::string_view romfile);
    inline unsigned char operator[](std::size_t i) const
    {
        return mRam[i];
    }
    inline void write(uint16_t address, uint8_t value, bool callback = true)
    {
        if (address >= 0x8000)
            mRam[address] = value;
        if (mWriteCallbacks[address] && callback)
            mWriteCallbacks[address](*this, address, value);
        if (address == 0xFF02)
        {
            if ((value & 0x80) == 0x80)
            {
                std::cout << mRam[0xFF01];
                mRam[0xFF01] &= 0xEF;
            }
        }
    }
    auto begin()
    {
        return (mRam.begin());
    }
    auto LoadSaveRAM() -> void;
    auto SaveRAM() -> void;
    auto RegisterCallback(uint16_t address, RamCallback fnc) -> void;

  private:
    std::string mFilename;
    std::array<unsigned char, RAM_SIZE> mRam;
    std::vector<RamCallback> mWriteCallbacks;
};

#endif