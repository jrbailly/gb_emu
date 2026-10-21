#ifndef _IFRONTEND_H_
#define _IFRONTEND_H_

#include "emulator.h"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

class IFrontend
{
  public:
    virtual ~IFrontend() = default;
    virtual auto init () -> void = 0;
    virtual auto get_input(uint8_t &pad, uint8_t &button) -> void = 0;
    virtual auto process_events() -> void = 0;
    virtual auto pop_save_request() -> bool = 0;
    virtual auto pop_load_request() -> bool = 0;
    virtual auto pop_rom_request(std::string &filename) -> bool = 0;
    virtual auto delay() -> void = 0;
    virtual auto play_audio(std::span<const int16_t> buffer) -> void = 0;
    virtual auto render(std::span<const uint8_t> buffer) -> void = 0;
    virtual auto get_samplerate() -> uint32_t = 0;
    virtual auto log(std::string_view message) -> void = 0;
};

#endif
