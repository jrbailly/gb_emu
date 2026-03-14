#include "apu.h"
#include "ram.h"
#include <array>
#include <format>
/**
 * @brief Constructor for the APU class.
 * @param ram Reference to the RamBus object for memory access.
 */
APU::APU(RamBus &ram) : _ram(ram)
{
    _next_cycle = SAMPLE_PERIOD;
    _timer_cycle = TIMER_PERIOD;
    _timer_count = 0;
    _buffer_index = 0;
    _audio_ready_size = 0;
    _duty_cycles[0] = 0.12;
    _duty_cycles[1] = 0.25;
    _duty_cycles[2] = 0.5;
    _duty_cycles[3] = 0.75;
    _channels[0].process = std::bind(&APU::process_ch1, this);
    _channels[1].process = std::bind(&APU::process_ch2, this);
    _channels[2].process = std::bind(&APU::process_ch3, this);
    _channels[3].process = std::bind(&APU::process_ch4, this);
    _channels[0].trigger = std::bind(&APU::trigger_ch1, this);
    _channels[1].trigger = std::bind(&APU::trigger_ch2, this);
    _channels[2].trigger = std::bind(&APU::trigger_ch3, this);
    _channels[3].trigger = std::bind(&APU::trigger_ch4, this);
}

/**
 * @brief Initializes the APU by registering memory callbacks for sound control registers.
 * @param ram Reference to the RamBus object for registering callbacks.
 */
auto APU::init(RamBus &ram) -> void
{
    int reg = Register::NR14;
    for (int i = 0; i < 4; ++i)
    {
        ram.register_callback(reg, [this, i](RamBus &, int, unsigned char val) {
            if (val & 0x80)
                trigger(i);
        });
        reg += 0x05;
    }
    ram.register_callback(Register::NR30, [](RamBus &ram, int, unsigned char val) {
        if ((val & 0x80) == 0)
            ram.write_register(NR52, ram[NR52] & ~(1 << 3));
    });
}

/**
 * @brief Steps the APU simulation by a given number of CPU cycles, processing audio and updating timers.
 * @param cycles_count The number of CPU cycles to advance the APU.
 */
auto APU::step(uint32_t cycles_count) -> void
{
    _next_cycle -= cycles_count;
    _timer_cycle -= cycles_count;
    if (_next_cycle <= 0)
    {
        if (_ram[NR52] & 0x80)
            for (int i = 0; i < 4; ++i)
                if (_ram[NR52] & (1 << i))
                    _channels[i].process();
        mixer();
        _next_cycle += SAMPLE_PERIOD;
    }
    if (_timer_cycle <= 0)
    {
        update_timer();
        if ((_timer_count % SWEEP_DIV) == 0)
            update_sweep();
        if ((_timer_count % ENVELOPPE_DIV) == 0)
            update_enveloppe();
        _timer_count++;
        _timer_cycle += TIMER_PERIOD;
    }
}

/**
 * @brief Finalizes the audio buffer for the current frame.
 */
auto APU::flush() -> void
{
    _audio_ready_size = _buffer_index;
    _buffer_index = 0;
}

/**
 * @brief Reload channels , trigger if needed
 *
 */
auto APU::load_state() -> void
{
    int reg = Register::NR14;

    _ram.write_register(Register::NR52, 0x80);
    for (int i = 0; i < 4; ++i)
    {
        if (_ram[reg] & 0x80)
            trigger(i);
        reg += 0x05;
    }
}

/**
 * @brief Processes the audio output for channel 1 (square wave with sweep).
 */
auto APU::process_ch1() -> void
{
    int duty = _ram[NR11] >> 6;
    int16_t value = 0;

    if (_channels[0].phase >= 1.0)
        _channels[0].phase -= 1.0;
    if (_channels[0].phase > _duty_cycles[duty])
        value = _channels[0].volume;
    _channels[0].value = value;
    _channels[0].phase += _channels[0].increment;
}

/**
 * @brief Processes the audio output for channel 2 (square wave).
 */
auto APU::process_ch2() -> void
{
    int duty = _ram[NR21] >> 6;
    int16_t value = 0;

    if (_channels[1].phase >= 1.0)
        _channels[1].phase -= 1.0;
    if (_channels[1].phase > _duty_cycles[duty])
        value = _channels[1].volume;
    _channels[1].value = value;
    _channels[1].phase += _channels[1].increment;
}

/**
 * @brief Processes the audio output for channel 3 (wave channel).
 */
auto APU::process_ch3() -> void
{
    int index = 0;
    int16_t value = 0;

    index = _channels[2].phase * PCM_SAMPLES;
    if ((index % 2) == 0)
        value = (_ram[WAVE_RAM + (index / 2)] >> 4) & 0xF;
    else
        value = _ram[WAVE_RAM + (index / 2)] & 0xF;
    value >>= _channels[2].volume;
    _channels[2].value = value;
    _channels[2].phase += _channels[2].increment;
    if (_channels[2].phase >= 1.0)
        _channels[2].phase -= 1.0;
}

/**
 * @brief Processes the audio output for channel 4 (noise channel).
 */
auto APU::process_ch4() -> void
{
    int feedback;
    int16_t value = 0;
    float count = _channels[3].increment;

    while (count > 0)
    {
        feedback = (_lfsr ^ (_lfsr >> 1)) & 1;
        _lfsr = (_lfsr >> 1) | (feedback << 15);
        if (_ram[NR43] & 0x08)
            _lfsr = (_lfsr & 0xFF7F) | (feedback << 7);
        count -= 1.0;
    }
    if ((_lfsr & 0x1) == 0)
        value = _channels[3].volume;
    _channels[3].value = value;
}

/**
 * @brief Triggers for all channels.
 */
auto APU::trigger(int channel) -> void
{
    int reg_channel_space = channel * 0x5;
    int sweep_pace = _ram[reg_channel_space + NR12] & 0x7;

    _ram.write_register(NR52, _ram[NR52] | (0x1 << channel));
    _channels[channel].phase = 0;
    _channels[channel].sweep_pace = sweep_pace;
    _channels[channel].direction = _ram[reg_channel_space + NR12] & 0x8;
    _channels[channel].length_timer = 512;
    _channels[channel].volume = _ram[reg_channel_space + NR12] >> 4;
    if (_ram[reg_channel_space + NR14] & 0x40)
        _channels[channel].length_timer = _ram[reg_channel_space + NR11] & 0x3F;
    _channels[channel].trigger();
}

/**
 * @brief Triggers the start of sound playback for channel 1.
 */
auto APU::trigger_ch1() -> void
{
    int sweep_time = (_ram[NR10] >> 4) & 0x7;
    int period = ((_ram[NR14] & 0x7) << 8) | _ram[NR13];

    _channels[0].increment = ((APU_FREQ / PULSE_SAMPLES) / (2048.0 - period)) / SAMPLERATE;
    _channels[0].sweep_count = sweep_time;
}

/**
 * @brief Triggers the start of sound playback for channel 2.
 */
auto APU::trigger_ch2() -> void
{
    int period = ((_ram[NR24] & 0x7) << 8) | _ram[NR23];

    _channels[1].increment = ((APU_FREQ / PULSE_SAMPLES) / (2048.0 - period)) / SAMPLERATE;
}

/**
 * @brief Triggers the start of sound playback for channel 3.
 */
auto APU::trigger_ch3() -> void
{
    int period = ((_ram[NR34] & 0x7) << 8) | _ram[NR33];

    _channels[2].increment = (65536.0 / (2048.0 - period)) / SAMPLERATE;
    _channels[2].sweep_pace = 0;
    if (_ram[NR34] & 0x40)
        _channels[2].length_timer = _ram[NR31];
    switch ((_ram[NR32] >> 5) & 0x3)
    {
    case 0:
        _channels[2].volume = 4;
        break;
    case 1:
        _channels[2].volume = 0;
        break;
    case 2:
        _channels[2].volume = 1;
        break;
    case 3:
        _channels[2].volume = 2;
        break;
    };
}

/**
 * @brief Triggers the start of sound playback for channel 4.
 */
auto APU::trigger_ch4() -> void
{
    int clock_shift = _ram[NR43] >> 4;
    int divider = _ram[NR43] & 0x7;

    if (divider == 0)
        divider = 1;
    _channels[3].increment = (262144 / (divider * (1 << clock_shift))) / SAMPLERATE;
    _lfsr = 0xFFFF;
}

/**
 * @brief Updates the frequency of channel 1.
 */
auto APU::update_sweep() -> void
{
    int freq;
    int step;
    int sweep_count = _timer_count / SWEEP_DIV;

    if (_channels[0].sweep_count > 0 && (sweep_count % _channels[0].sweep_count) == 0)
    {
        freq = ((_ram[NR14] & 0x7) << 8) | _ram[NR13];
        step = _ram[NR10] & 0x7;
        if (_ram[NR10] & 0x08)
            freq = freq - (freq / (1 << step));
        else
            freq = freq + (freq / (1 << step));
        _channels[0].increment = ((APU_FREQ / PULSE_SAMPLES) / (2048.0 - freq)) / SAMPLERATE;
        if (freq > 0x7FF)
            _ram.write_register(NR52, _ram[NR52] & 0xFE);
        _ram.write_register(Register::NR13, freq & 0xFF);
        _ram.write_register(Register::NR14, (freq >> 8) & 0x7);
    }
}

/**
 * @brief Updates the volume envelope for all active sound channels.
 */
auto APU::update_enveloppe() -> void
{
    int enveloppe_count = _timer_count / ENVELOPPE_DIV;

    for (int i = 0; i < 4; ++i)
    {
        if (_channels[i].sweep_pace > 0 && (enveloppe_count % _channels[i].sweep_pace) == 0)
        {
            if (_channels[i].direction)
                _channels[i].volume++;
            else
                _channels[i].volume--;
            if (_channels[i].volume < 0x0)
                _channels[i].volume = 0x0;
            if (_channels[i].volume > 0xF)
                _channels[i].volume = 0xF;
        }
    }
}

/**
 * @brief Updates the length timer for all active sound channels.
 */
auto APU::update_timer() -> void
{
    for (int i = 0; i < 4; ++i)
    {
        _channels[i].length_timer++;
        if ((i == 2 && _channels[i].length_timer == 256) || (i != 2 && _channels[i].length_timer == 64))
            _ram.write_register(NR52, _ram[NR52] & ~(1 << i));
    }
}

/**
 * @brief Mixes the audio output from the four channels into the audio buffer.
 */
auto APU::mixer() -> void
{
    int16_t value[CHANNELS] = {0, 0};
    int panning = _ram[NR51];
    int master_volume = _ram[NR50];

    for (int i = 0; i < CHANNELS; i++)
    {
        value[i] = 0;
        for (int j = 0; j < 4; ++j)
        {
            if ((panning & (1 << j)) && (_ram[NR52] & (1 << j)))
                value[i] += _channels[j].value;
        }
        value[i] *= 1 + (master_volume & 0x7);
        panning >>= 4;
        master_volume >>= 4;
    }
    _buffer[_buffer_index] = -(value[1] * 16.0);
    _buffer[_buffer_index + 1] = -(value[0] * 16.0);
    _buffer_index = (_buffer_index + 2) % AUDIO_BUFFER_SIZE;
}
