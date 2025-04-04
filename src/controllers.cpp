#include "controllers.h"

Controllers::Controllers() : mDpads(0xf), mButtons(0xf)
{
    _pads_binding[SDLK_UP] = UP;
    _pads_binding[SDL_GAMEPAD_BUTTON_DPAD_UP] = UP;
    _pads_binding[SDLK_DOWN] = DOWN;
    _pads_binding[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = DOWN;
    _pads_binding[SDLK_LEFT] = LEFT;
    _pads_binding[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = LEFT;
    _pads_binding[SDLK_RIGHT] = RIGHT;
    _pads_binding[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = RIGHT;
    _buttons_binding[SDLK_RETURN] = START;
    _buttons_binding[SDL_GAMEPAD_BUTTON_START] = START;
    _buttons_binding[SDLK_BACKSPACE] = SELECT;
    _buttons_binding[SDL_GAMEPAD_BUTTON_GUIDE] = SELECT;
    _buttons_binding[SDLK_LCTRL] = A;
    _buttons_binding[SDL_GAMEPAD_BUTTON_SOUTH] = A;
    _buttons_binding[SDLK_LALT] = B;
    _buttons_binding[SDL_GAMEPAD_BUTTON_EAST] = B;
}

auto Controllers::init(RamBus &ram) -> void
{
    ram.register_callback(Register::JOYP, [this](RamBus &ram, int, unsigned char val) {
        uint8_t value = val & 0x30;

        if (value & 0x20 && mDpads != 0xF)
            ram.write_register(Register::JOYP, 0x20 | mDpads);
        else if (value & 0x10 && mButtons != 0xF)
            ram.write_register(Register::JOYP, 0x10 | mButtons);
        else
            ram.write_register(Register::JOYP, 0x3F);
    });
}

auto Controllers::setInput(int key, bool down) -> void
{
    if (_pads_binding.find(key) != _pads_binding.end())
    {
        if (down)
            mDpads &= ~_pads_binding[key];
        else
            mDpads |= _pads_binding[key];
    }
    if (_buttons_binding.find(key) != _buttons_binding.end())
    {
        if (down)
            mButtons &= ~_buttons_binding[key];
        else
            mButtons |= _buttons_binding[key];
    }
}