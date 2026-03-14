#include "controllers.h"
#include "cpu.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

Controllers::Controllers(RamBus &ram)
    : _ram(ram), _dpads(0xf), _buttons(0xf), _record_index(0), _next_record(-1), _play_record(false)
{
    _dpads_binding[SDLK_UP] = UP;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_UP] = UP;
    _dpads_binding[SDLK_DOWN] = DOWN;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = DOWN;
    _dpads_binding[SDLK_LEFT] = LEFT;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = LEFT;
    _dpads_binding[SDLK_RIGHT] = RIGHT;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = RIGHT;
    _buttons_binding[SDLK_RETURN] = START;
    _buttons_binding[SDL_GAMEPAD_BUTTON_START] = START;
    _buttons_binding[SDLK_BACKSPACE] = SELECT;
    _buttons_binding[SDL_GAMEPAD_BUTTON_GUIDE] = SELECT;
    _buttons_binding[SDLK_LCTRL] = A;
    _buttons_binding[SDL_GAMEPAD_BUTTON_SOUTH] = A;
    _buttons_binding[SDLK_LALT] = B;
    _buttons_binding[SDL_GAMEPAD_BUTTON_EAST] = B;
}

auto Controllers::init(RamBus &ram, const Config &config) -> void
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
    if (!config._recordfile.empty())
        parse_recordfile(config._recordfile);
}

auto Controllers::setInput(int key, bool down) -> void
{
    if (_play_record == false)
    {
        if (_dpads_binding.find(key) != _dpads_binding.end())
        {
            if (down)
                _dpads &= ~_dpads_binding[key];
            else
                _dpads |= _dpads_binding[key];
        }
        if (_buttons_binding.find(key) != _buttons_binding.end())
        {
            if (down)
                _buttons &= ~_buttons_binding[key];
            else
                _buttons |= _buttons_binding[key];
        }
    }
    if (down)
        _ram.write_register(CPU::Register::IF, _ram[CPU::Register::IF] | 0x10);
}

auto Controllers::step(uint32_t cycles) -> void
{
    int last_dpads = _dpads;
    int last_buttons = _buttons;
    bool active_interrupt = false;

    _next_record -= cycles;
    if (_play_record && _next_record <= 0)
    {
        _dpads = _dpads_records[_record_index];
        _buttons = _buttons_records[_record_index];
        _record_index = (_record_index + 1) % _dpads_records.size();
        for (int i = 0; i < 4; ++i)
        {
            if (((last_dpads & (1 << i)) && ((_dpads & (1 << i)) == 0)) ||
                ((last_buttons & (1 << i)) && ((_buttons & (1 << i)) == 0)))
                active_interrupt = true;
        }
        if (active_interrupt)
            _ram.write_register(CPU::Register::IF, _ram[CPU::Register::IF] | 0x10);
        _next_record += RECORD_CYCLE;
    }
}

auto Controllers::parse_recordfile(const std::string &filename) -> void
{
    std::ifstream input(filename.data(), std::ios::binary);
    std::string line;
    int dpads;
    int buttons;

    if (!input)
        throw std::runtime_error(std::format("Failed to open record file: ") + filename.data());
    while (std::getline(input, line))
    {
        dpads = 0xF;
        buttons = 0xF;
        if (line.starts_with('|'))
        {
            if (line[1] != '.')
                dpads &= ~(UP);
            if (line[2] != '.')
                dpads &= ~(DOWN);
            if (line[3] != '.')
                dpads &= ~(LEFT);
            if (line[4] != '.')
                dpads &= ~(RIGHT);
            if (line[5] != '.')
                buttons &= ~(START);
            if (line[6] != '.')
                buttons &= ~(SELECT);
            if (line[7] != '.')
                buttons &= ~(B);
            if (line[8] != '.')
                buttons &= ~(A);
            _dpads_records.push_back(dpads);
            _buttons_records.push_back(buttons);
        }
    }
    _play_record = true;
}