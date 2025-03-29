#ifndef _APU_H_
#define _APU_H_
#include "cpu.h"
#include <SDL3/SDL.h>

static constexpr float APU_FREQ = CPU_FREQ / 4.0;
static constexpr float SAMPLERATE = 44100;
static constexpr float SAMPLE_PERIOD = CPU_FREQ / SAMPLERATE;
static constexpr float TIMER_PERIOD = CPU_FREQ / 256;
static constexpr float ENVELOPPE_PERIOD = CPU_FREQ / 64;
static constexpr float PULSE_SAMPLES = 8;
static constexpr float PCM_SAMPLES = 32;
static constexpr int CHANNELS = 2;
static constexpr int AUDIO_BUFFER_SIZE = 65536;

struct Channel
{
    float phase;
    float increment;
    int length_timer;
    int enveloppe_count;
    int sweep_pace;
    int direction;
    int volume;
    int16_t value;
};

class APU
{
  public:
    enum Register
    {
        NR10 = 0xFF10,
        NR11 = 0xFF11,
        NR12 = 0xFF12,
        NR13 = 0xFF13,
        NR14 = 0xFF14,
        NR21 = 0xFF16,
        NR22 = 0xFF17,
        NR23 = 0xFF18,
        NR24 = 0xFF19,
        NR30 = 0xFF1A,
        NR31 = 0xFF1B,
        NR32 = 0xFF1C,
        NR33 = 0xFF1D,
        NR34 = 0xFF1E,
        NR41 = 0xFF20,
        NR42 = 0xFF21,
        NR43 = 0xFF22,
        NR44 = 0xFF23,
        NR50 = 0xFF24,
        NR51 = 0xFF25,
        NR52 = 0xFF26,
        WAVE_RAM = 0xFF30,
    };
    APU(RamBus &ram);
    auto init(RamBus &ram) -> void;
    auto step(uint32_t cycles_count) -> void;
    auto flush() -> void;

  private:
    auto init_sdl() -> void;
    auto process_ch1() -> void;
    auto process_ch2() -> void;
    auto process_ch3() -> void;
    auto process_ch4() -> void;
    auto trigger_ch1() -> void;
    auto trigger_ch2() -> void;
    auto trigger_ch3() -> void;
    auto trigger_ch4() -> void;
    auto update_enveloppe() -> void;
    auto update_timer() -> void;
    auto mixer() -> void;
    auto highpass_filter(float v, int channel) -> float;
    auto dc_removal() -> void;

  private:
    RamBus &_ram;
    int _next_cycle;
    int _sweep_cycle;
    int _timer_cycle;
    int _enveloppe_cycle;
    int _buffer_index;
    uint16_t _lfsr;
    SDL_AudioStream *_audio_stream;
    std::array<float, 4> _duty_cycles;
    std::array<Channel, 4> _channels;
    std::array<std::array<float, 4>, CHANNELS> _hfilter_x;
    std::array<std::array<float, 4>, CHANNELS> _hfilter_y;
    std::array<int16_t, AUDIO_BUFFER_SIZE * CHANNELS> _buffer;
};

#endif