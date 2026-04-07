#ifndef _IFRONTEND_H_
#define _IFRONTEND_H_

#include "emulator.h"
#include <cstdint>
#include <span>

class IFrontend
{
  public:
    virtual ~IFrontend() = default;
    virtual auto get_input(uint8_t &pad, uint8_t &button) -> bool = 0;
    virtual auto pop_save_request() -> bool = 0;
    virtual auto pop_load_request() -> bool = 0;
    virtual auto delay() -> void = 0;
    virtual auto play_audio(std::span<const int16_t> buffer) -> void = 0;
    virtual auto render(const SaveState &state) -> void = 0;
    virtual auto get_samplerate() -> uint32_t = 0;
};

#endif
