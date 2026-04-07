#ifndef _RAM_H_
#define _RAM_H_
#include "iserializable.h"
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

class RamBus : public ISerializable
{
  public:
    RamBus(bool unittest = false);
    auto clear() -> void;
    auto save_state(StateMap &state) -> void override;
    auto load_state(const StateMap &state) -> void override;
    inline auto operator[](std::size_t i) const -> unsigned char
    {
        return _ram[i];
    }
    inline auto write(int address, unsigned char value) -> void
    {
        if (address >= 0x8000 || _unittest)
            _ram[address] = value;
        if (_write_callbacks[address])
            _write_callbacks[address](*this, address, value);
    }
    inline auto write_register(int address, unsigned char value) -> void
    {
        _ram[address] = value;
    }
    auto read_range(unsigned int address, size_t size, unsigned char *dst_datas) -> void;
    auto write_range(const unsigned char *datas, size_t size, unsigned int address) -> void;
    auto write_range(unsigned int start_address, unsigned int dst_address, size_t size) -> void;
    auto register_callback(int address, ram_callback fnc) -> void;
    auto register_callback_range(int start_address, size_t size, ram_callback fnc) -> void;
    const std::array<unsigned char, Ram::ram_size> getDatas() const
    {
        return _ram;
    }

  private:
    bool _unittest;
    std::array<unsigned char, Ram::ram_size> _ram;
    std::vector<ram_callback> _write_callbacks;
};

#endif