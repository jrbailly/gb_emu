#ifndef _CONTROLLERS_H_
#define _CONTROLLERS_H_

#include "ram.h"
#include <SDL3/SDL.h>

class Controllers
{
  public:
    enum Register
    {
        JOYP = 0xFF00,
    };
    enum DPadMask
    {
        RIGHT = 0x01,
        LEFT = 0x02,
        UP = 0x04,
        DOWN = 0x08,
    };
    enum DButtonMask
    {
        A = 0x01,
        B = 0x02,
        SELECT = 0x04,
        START = 0x08,
    };

    Controllers(MBC1 &ram);
    void step();
    void setInput(SDL_GamepadButtonEvent &event);
    void setInput(SDL_KeyboardEvent &event);

  private:
    uint8_t mDpads;
    uint8_t mButtons;
    MBC1 &mRam;
};

#endif