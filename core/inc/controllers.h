#ifndef _CONTROLLERS_H_
#define _CONTROLLERS_H_

#include "iserializable.h"
#include "ram.h"
#include <map>

class Controllers : public ISerializable
{
  public:
    enum Register
    {
        JOYP = 0xFF00,
    };
    enum ESelect
    {
        BUTTON = 0x20,
        DPAD = 0x10,
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

    Controllers(RamBus &ram);
    auto init(RamBus &ram) -> void;
    auto set_input(int pad, int button) -> void;
    auto save_state(StateMap &state) -> void override;
    auto load_state(const StateMap &state) -> void override;

  private:
    RamBus &_ram;
    int _dpads;
    int _buttons;
};

#endif
