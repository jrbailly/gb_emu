#ifndef _APU_H_
#define _APU_H_
#include "cpu.h"
#include "iserializable.h"
#include <array>
#include <cstdint>
#include <span>

static constexpr float apu_freq = cpu_freq / 4.0;
static constexpr int timer_period = cpu_freq / 256;
static constexpr int sweep_div = 2;
static constexpr int enveloppe_div = 4;
static constexpr float pulse_samples = 8;
static constexpr float pcm_samples = 32;
static constexpr int channels = 2;
static constexpr int audio_buffer_size = 65536;

struct Channel
{
    float phase = 0;
    float increment = 0;
    int length_timer = 0;
    int sweep_count = 0;
    int sweep_pace = 0;
    int direction = 0;
    int volume = 0;
    int16_t value = 0;
    bool length_enabled = false;
    std::function<void()> trigger;
    std::function<void()> process;
};

class APU : public ISerializable
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
    auto set_samplerate(float samplerate) -> void;
    auto step(uint32_t cycles_count) -> void;
    auto flush() -> void;
    auto save_state(StateMap &state) -> void override;
    auto load_state(const StateMap &state) -> void override;
    inline auto get_audio_buffer() const -> std::span<const int16_t>
    {
        return {_buffer.data(), _buffer_index};
    }

  private:
    auto process_ch1() -> void;
    auto process_ch2() -> void;
    auto process_ch3() -> void;
    auto process_ch4() -> void;
    auto trigger(int channel) -> void;
    auto trigger_ch1() -> void;
    auto trigger_ch2() -> void;
    auto trigger_ch3() -> void;
    auto trigger_ch4() -> void;
    auto update_sweep() -> void;
    auto update_enveloppe() -> void;
    auto update_timer() -> void;
    auto mixer() -> void;

  private:
    RamBus &_ram;
    float _samplerate;
    float _sample_period;
    float _next_cycle;
    int _timer_cycle;
    int _timer_count;
    std::size_t _buffer_index;
    uint16_t _lfsr;
    std::array<float, 4> _duty_cycles;
    std::array<Channel, 4> _channels;
    std::array<int16_t, audio_buffer_size * channels> _buffer;
};

#endif