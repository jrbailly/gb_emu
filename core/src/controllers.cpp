#include "controllers.h"
#include "cpu.h"

Controllers::Controllers(RamBus &ram) : _ram(ram), _dpads(0xf), _buttons(0xf)
{
}

auto Controllers::init(RamBus &ram) -> void
{
    ram.register_callback(Register::JOYP, [this](RamBus &ram, int, unsigned char val) {
        uint8_t select = val & (ESelect::BUTTON | ESelect::DPAD);
        uint8_t low;

        if (select == 0x00)
            low = _buttons & _dpads;
        else if ((select & ESelect::BUTTON) == 0)
            low = _buttons;
        else if ((select & ESelect::DPAD) == 0)
            low = _dpads;
        else
            low = 0x0F;

        ram.write_register(Register::JOYP, 0xC0 | select | low);
    });
}

auto Controllers::set_input(int pad, int button) -> void
{
    if (_dpads != pad || _buttons != button)
        _ram.write_register(CPU::Register::IF, _ram[CPU::Register::IF] | CPU::IFFlag::JOYPAD);
    _dpads = pad;
    _buttons = button;
}
