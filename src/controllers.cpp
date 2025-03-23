#include "controllers.h"

Controllers::Controllers() : mDpads(0xf), mButtons(0xf)
{
}

auto Controllers::init(MBC1 &ram) -> void
{
    ram.RegisterCallback(Register::JOYP, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
        uint8_t value = val & 0x30;

        if (value & 0x20 && mDpads != 0xF)
            ram.write(Register::JOYP, 0x20 | mDpads, false);
        else if (value & 0x10 && mButtons != 0xF)
            ram.write(Register::JOYP, 0x10 | mButtons, false);
        else
            ram.write(Register::JOYP, 0x3F, false);
    });
}

void Controllers::setInput(SDL_GamepadButtonEvent &event)
{
    switch (event.button)
    {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        if (event.down)
            mDpads &= ~UP;
        else
            mDpads |= UP;
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        if (event.down)
            mDpads &= ~DOWN;
        else
            mDpads |= DOWN;
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        if (event.down)
            mDpads &= ~LEFT;
        else
            mDpads |= LEFT;
        break;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        if (event.down)
            mDpads &= ~RIGHT;
        else
            mDpads |= RIGHT;
        break;
    case SDL_GAMEPAD_BUTTON_START:
        if (event.down)
            mButtons &= ~START;
        else
            mButtons |= START;
        break;
    case SDL_GAMEPAD_BUTTON_GUIDE:
        if (event.down)
            mButtons &= ~SELECT;
        else
            mButtons |= SELECT;
        break;
    case SDL_GAMEPAD_BUTTON_SOUTH:
        if (event.down)
            mButtons &= ~A;
        else
            mButtons |= A;
        break;
    case SDL_GAMEPAD_BUTTON_EAST:
        if (event.down)
            mButtons &= ~B;
        else
            mButtons |= B;
        break;
    }
}

void Controllers::setInput(SDL_KeyboardEvent &event)
{
    switch (event.key)
    {
    case SDLK_UP:
        if (event.down)
            mDpads &= ~UP;
        else
            mDpads |= UP;
        break;
    case SDLK_DOWN:
        if (event.down)
            mDpads &= ~DOWN;
        else
            mDpads |= DOWN;
        break;
    case SDLK_LEFT:
        if (event.down)
            mDpads &= ~LEFT;
        else
            mDpads |= LEFT;
        break;
    case SDLK_RIGHT:
        if (event.down)
            mDpads &= ~RIGHT;
        else
            mDpads |= RIGHT;
        break;
    case SDLK_RETURN:
        if (event.down)
            mButtons &= ~START;
        else
            mButtons |= START;
        break;
    case SDLK_BACKSPACE:
        if (event.down)
            mButtons &= ~SELECT;
        else
            mButtons |= SELECT;
        break;
    case SDLK_LCTRL:
        if (event.down)
            mButtons &= ~A;
        else
            mButtons |= A;
        break;
    case SDLK_LALT:
        if (event.down)
            mButtons &= ~B;
        else
            mButtons |= B;
        break;
    }
}
