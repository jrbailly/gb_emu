#ifndef _CONTROLLERS_H_
#define _CONTROLLERS_H_

#include "ram.h"
#include <SDL3/SDL.h>
#include <map>

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

    Controllers();
    auto init(RamBus &ram) -> void;
    auto setInput(int key, bool down) -> void;

  private:
    int mDpads;
    int mButtons;
    std::map<int, int> _pads_binding;
    std::map<int, int> _buttons_binding;
};

#endif