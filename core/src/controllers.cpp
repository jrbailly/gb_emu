#include "controllers.h"
#include "cpu.h"

Controllers::Controllers(RamBus &ram) : _ram(ram), _dpads(0xf), _buttons(0xf)
{
}

auto Controllers::init(RamBus &ram) -> void
{
    ram.register_callback(Register::JOYP, [this](RamBus &ram, int, unsigned char val) {
        uint8_t value = val & 0xF0;

        if ((value & 0x20) == 0)
            value = 0x20 | _buttons;
        else if ((value & 0x10) == 0)
            value = 0x10 | _dpads;
        if (_dpads == 0xF && _buttons == 0xF)
            value = 0x3F;
        ram.write_register(Register::JOYP, value);
    });
}

auto Controllers::set_input(int pad, int button) -> void
{
    int last_dpads = _dpads;
    int last_buttons = _buttons;
    bool active_interrupt = false;

    _dpads = pad;
    _buttons = button;
    for (int i = 0; i < 4; ++i)
    {
        if (((last_dpads & (1 << i)) && ((_dpads & (1 << i)) == 0)) ||
            ((last_buttons & (1 << i)) && ((_buttons & (1 << i)) == 0)))
            active_interrupt = true;
    }
    if (active_interrupt)
        _ram.write_register(CPU::Register::IF, _ram[CPU::Register::IF] | 0x10);
}
