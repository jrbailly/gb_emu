#ifndef _RAM_H_
#define _RAM_H_
#include <array>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

class RamBus;
typedef std::function<void(RamBus &ram, int, unsigned char)> ram_callback;

struct Ram
{
    static constexpr std::size_t ram_size = 0x10000;
};

class RamBus
{
  public:
    RamBus();
    inline auto operator[](std::size_t i) const -> unsigned char
    {
        return _ram[i];
    }
    inline auto write(int address, unsigned char value, bool callback = true) -> void
    {
        if (address >= 0x8000)
            _ram[address] = value;
        if (_write_callbacks[address] && callback)
            _write_callbacks[address](*this, address, value);
        if (address == 0xFF02)
        {
            if ((value & 0x80) == 0x80)
            {
                std::cout << _ram[0xFF01];
                _ram[0xFF01] &= 0xEF;
            }
        }
    }
    auto write_range(const unsigned char *datas, int size, int address) -> void;
    auto load_ram(const std::string &romfile) -> void;
    auto save_ram(const std::string &romfile) -> void;
    auto register_callback(int address, ram_callback fnc) -> void;
    auto register_callback_range(int start_address, unsigned int size, ram_callback fnc) -> void;
    auto data()
    {
        return _ram.data();
    }

  private:
    std::array<unsigned char, Ram::ram_size> _ram;
    std::vector<ram_callback> _write_callbacks;
};

#endif