#include "apu.h"
#include "ram.h"
#include <array>

/**
 * @brief Constructor for the APU class.
 * @param ram Reference to the RamBus object for memory access.
 */
APU::APU(RamBus &ram) : _ram(ram)
{
    _next_cycle = SAMPLE_PERIOD;
    _sweep_cycle = 0;
    _timer_cycle = TIMER_PERIOD;
    _enveloppe_cycle = ENVELOPPE_PERIOD;
    _buffer_index = 0;
    _duty_cycles[0] = 0.12;
    _duty_cycles[1] = 0.25;
    _duty_cycles[2] = 0.5;
    _duty_cycles[3] = 0.75;
}

/**
 * @brief Initializes the SDL audio system for sound output.
 */
auto APU::init_sdl() -> void
{
    SDL_AudioSpec spec;

    spec.format = SDL_AUDIO_S16;
    spec.channels = CHANNELS;
    spec.freq = SAMPLERATE;
    _audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!_audio_stream)
        throw std::runtime_error(std::string("SDL_OpenAudioDeviceStream : ") + SDL_GetError());
    if (!SDL_ResumeAudioStreamDevice(_audio_stream))
        throw std::runtime_error(std::string("SDL_ResumeAudioStreamDevice : ") + SDL_GetError());
}

/**
 * @brief Initializes the APU by registering memory callbacks for sound control registers.
 * @param ram Reference to the RamBus object for registering callbacks.
 */
auto APU::init(RamBus &ram) -> void
{
    ram.register_callback(Register::NR14, [this](RamBus &ram, int addr, unsigned char val) {
        if (val & 0x80)
            trigger_ch1();
    });
    ram.register_callback(Register::NR24, [this](RamBus &ram, int addr, unsigned char val) {
        if (val & 0x80)
            trigger_ch2();
    });
    ram.register_callback(Register::NR34, [this](RamBus &ram, int addr, unsigned char val) {
        if (val & 0x80)
            trigger_ch3();
    });
    ram.register_callback(Register::NR44, [this](RamBus &ram, int addr, unsigned char val) {
        if (val & 0x80)
            trigger_ch4();
    });
    ram.register_callback(Register::NR30, [this](RamBus &ram, int addr, unsigned char val) {
        if ((val & 0x80) == 0)
            ram.write_register(NR52, ram[NR52] & ~(1 << 3));
    });
    init_sdl();
}

/**
 * @brief Steps the APU simulation by a given number of CPU cycles, processing audio and updating timers.
 * @param cycles_count The number of CPU cycles to advance the APU.
 */
auto APU::step(uint32_t cycles_count) -> void
{
    _next_cycle -= cycles_count;
    _sweep_cycle -= cycles_count;
    _timer_cycle -= cycles_count;
    _enveloppe_cycle -= cycles_count;
    if (_next_cycle <= 0)
    {
        if (_ram[NR52] & 0x80)
        {
            if (_ram[NR52] & 0x1)
                process_ch1();
            if (_ram[NR52] & 0x2)
                process_ch2();
            if (_ram[NR52] & 0x4)
                process_ch3();
            if (_ram[NR52] & 0x8)
                process_ch4();
        }
        mixer();
        _next_cycle += SAMPLE_PERIOD;
    }
    update_enveloppe();
    update_timer();
}

/**
 * @brief Flushes the current audio buffer to the SDL audio stream and performs DC removal.
 */
auto APU::flush() -> void
{
    dc_removal();
    if (!SDL_PutAudioStreamData(_audio_stream, _buffer.data(), _buffer_index * sizeof(int16_t)))
        throw std::runtime_error(std::string("SDL_PutAudioStreamData : ") + SDL_GetError());
    _buffer_index = 0;
}

/**
 * @brief Processes the audio output for channel 1 (square wave with sweep).
 */
auto APU::process_ch1() -> void
{
    int freq;
    int duty = _ram[NR11] >> 6;
    int sweep_time = (_ram[NR10] >> 4) & 0x7;
    int step = _ram[NR10] & 0x7;
    int16_t value = 0;

    if (sweep_time > 0 && _sweep_cycle <= 0)
    {
        freq = ((_ram[NR14] & 0x7) << 8) | _ram[NR13];
        if (_ram[NR10] & 0x08)
            freq = freq - (freq / (1 << step));
        else
            freq = freq + (freq / (1 << step));
        _channels[0].increment = ((APU_FREQ / PULSE_SAMPLES) / (2048.0 - freq)) / SAMPLERATE;
        _sweep_cycle += (CPU_FREQ / 128) * sweep_time;
        if (freq > 0x7FF)
            _ram.write_register(NR52, _ram[NR52] & 0xFE);
        _ram.write_register(Register::NR13, freq & 0xFF);
        _ram.write_register(Register::NR14, (freq >> 8) & 0x7);
    }
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
 * @brief Triggers the start of sound playback for channel 1.
 */
auto APU::trigger_ch1() -> void
{
    int sweep_pace = _ram[NR12] & 0x7;
    int sweep_time = (_ram[NR10] >> 4) & 0x7;
    int period = ((_ram[NR14] & 0x7) << 8) | _ram[NR13];

    _ram.write_register(NR52, _ram[NR52] | 0x1);
    _channels[0].phase = 0;
    _channels[0].increment = ((APU_FREQ / PULSE_SAMPLES) / (2048.0 - period)) / SAMPLERATE;
    _channels[0].enveloppe_count = 0;
    _channels[0].sweep_pace = sweep_pace;
    _channels[0].direction = _ram[NR12] & 0x8;
    _channels[0].length_timer = 255;
    _channels[0].volume = _ram[NR12] >> 4;
    _sweep_cycle = (CPU_FREQ / 128) * sweep_time;
    if (_ram[NR14] & 0x40)
        _channels[0].length_timer = _ram[NR11] & 0x3F;
}

/**
 * @brief Triggers the start of sound playback for channel 2.
 */
auto APU::trigger_ch2() -> void
{
    int sweep_pace = _ram[NR22] & 0x7;
    int period = ((_ram[NR24] & 0x7) << 8) | _ram[NR23];

    _ram.write_register(NR52, _ram[NR52] | 0x2);
    _channels[1].phase = 0;
    _channels[1].increment = ((APU_FREQ / PULSE_SAMPLES) / (2048.0 - period)) / SAMPLERATE;
    _channels[1].enveloppe_count = 0;
    _channels[1].sweep_pace = sweep_pace;
    _channels[1].direction = _ram[NR22] & 0x8;
    _channels[1].length_timer = 255;
    _channels[1].volume = _ram[NR22] >> 4;
    if (_ram[NR24] & 0x40)
        _channels[1].length_timer = _ram[NR21] & 0x3F;
}

/**
 * @brief Triggers the start of sound playback for channel 3.
 */
auto APU::trigger_ch3() -> void
{
    int sweep_pace = _ram[NR31];
    int period = ((_ram[NR34] & 0x7) << 8) | _ram[NR33];

    _ram.write_register(NR52, _ram[NR52] | 0x4);
    _channels[2].phase = 0;
    _channels[2].increment = (65536.0 / (2048.0 - period)) / SAMPLERATE;
    _channels[2].length_timer = 512;
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
    int sweep_pace = _ram[NR42] & 0x7;
    int volume = (_ram[NR42] >> 4);

    if (divider == 0)
        divider = 1;
    _ram.write_register(NR52, _ram[NR52] | 0x8);
    _channels[3].enveloppe_count = 0;
    _channels[3].sweep_pace = sweep_pace;
    _channels[3].direction = _ram[NR42] & 0x8;
    _channels[3].length_timer = 512;
    _channels[3].phase = 0;
    _channels[3].increment = (262144 / (divider * (1 << clock_shift))) / SAMPLERATE;
    _channels[3].volume = _ram[NR42] >> 4;
    if (_ram[NR44] & 0x40)
        _channels[3].length_timer = _ram[NR41] & 0x3F;
    _lfsr = 0xFFFF;
}

/**
 * @brief Updates the volume envelope for all active sound channels.
 */
auto APU::update_enveloppe() -> void
{
    if (_enveloppe_cycle <= 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            _channels[i].enveloppe_count++;
            if (_channels[i].enveloppe_count == _channels[i].sweep_pace)
            {
                if (_channels[i].direction)
                    _channels[i].volume++;
                else
                    _channels[i].volume--;
                if (_channels[i].volume < 0x0)
                    _channels[i].volume = 0x0;
                if (_channels[i].volume > 0xF)
                    _channels[i].volume = 0xF;
                _channels[i].enveloppe_count = 0;
            }
        }
        _enveloppe_cycle += ENVELOPPE_PERIOD;
    }
}

/**
 * @brief Updates the length timer for all active sound channels.
 */
auto APU::update_timer() -> void
{
    if (_timer_cycle <= 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            _channels[i].length_timer++;
            if ((i == 2 && _channels[i].length_timer == 256) || (i != 2 && _channels[i].length_timer == 64))
                _ram.write_register(NR52, _ram[NR52] & ~(1 << i));
        }
        _timer_cycle += TIMER_PERIOD;
    }
}

/**
 * @brief Mixes the audio output from the four channels into the audio buffer.
 */
auto APU::mixer() -> void
{
    int16_t value[2] = {0, 0};
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
    _buffer[_buffer_index] = -(value[1] * 16);
    _buffer[_buffer_index + 1] = -(value[0] * 16);
    _buffer_index = (_buffer_index + 2) % AUDIO_BUFFER_SIZE;
}

/**
 * @brief Applies a Butterworth 3 order high-pass filter to the audio signal with a cutoff frequeny of 50hz.
 * @param v The input audio sample value.
 * @param channel The audio channel (0 or 1).
 * @return The filtered audio sample value.
 */
auto APU::highpass_filter(float v, int channel) -> float
{
    int i;
    float out = 0;
    float acoeff[] = {-0.9858534011017497, 2.9716062044560796, -2.985752444394766, 1};
    float bcoeff[] = {-1, 3, -3, 1};
    float gain = 1.0071492426099524;

    for (i = 0; i < 3; i++)
    {
        _hfilter_x[channel][i] = _hfilter_x[channel][i + 1];
    }
    _hfilter_x[channel][3] = v / gain;
    for (i = 0; i < 3; i++)
    {
        _hfilter_y[channel][i] = _hfilter_y[channel][i + 1];
    }
    for (i = 0; i <= 3; i++)
    {
        out += _hfilter_x[channel][i] * bcoeff[i];
    }
    for (i = 0; i < 3; i++)
    {
        out -= _hfilter_y[channel][i] * acoeff[i];
    }
    _hfilter_y[channel][3] = out;
    return out;
}

/**
 * @brief Removes the DC offset from the audio buffer using a high-pass filter.
 */
auto APU::dc_removal() -> void
{
    for (int channel = 0; channel < CHANNELS; ++channel)
        for (int i = 0; i < _buffer_index; i += 2)
            _buffer[i] = highpass_filter(_buffer[i], channel);
}
