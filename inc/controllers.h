#ifndef _CONTROLLERS_H_
#define _CONTROLLERS_H_

#include "config.h"
#include "ram.h"
#include <SDL3/SDL.h>
#include <map>
#include <vector>

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
    auto init(RamBus &ram, const Config &config) -> void;
    auto setInput(int key, bool down) -> void;
    auto step() -> void;

  private:
    auto parse_recordfile(const std::string &filename) -> void;

  private:
    int _dpads;
    int _buttons;
    int _record_index;
    bool _play_record;
    std::map<int, int> _dpads_binding;
    std::map<int, int> _buttons_binding;
    std::vector<int> _dpads_records;
    std::vector<int> _buttons_records;
};

#endif