#ifndef _EMULATOR_H_
#define _EMULATOR_H_

#include "apu.h"
#include "cartridge.h"
#include "config.h"
#include "controllers.h"
#include "cpu.h"
#include "lcd.h"
#include "ram.h"
#include "timer.h"
#include <memory>
#include <span>

static constexpr int refresh_rate = 60;
static constexpr int frame_cycle_count = CPU_FREQ / refresh_rate;
static constexpr int frame_duration = (1000000 / refresh_rate);

class Emulator
{
  public:
    Emulator(const Config &Configuration);
    auto init() -> void;
    auto step_frame() -> int;
    auto set_input(int pad, int button) -> void;
    auto save_state() -> void;
    auto load_state() -> void;
    inline auto get_audio_buffer() const -> APU::AudioView
    {
        return _apu->get_audio_buffer();
    }
    inline auto get_frame_buffer() const -> std::span<const uint32_t>
    {
        return _lcd->get_frame_buffer();
    }

  private:
    RamBus _ram;
    std::unique_ptr<APU> _apu;
    std::unique_ptr<Cartridge> _cartridge;
    std::unique_ptr<Controllers> _controllers;
    std::unique_ptr<CPU> _cpu;
    std::unique_ptr<LCD> _lcd;
    std::unique_ptr<Timer> _timer;
    const Config _config;
    uint32_t _cycles_count;
};

#endif