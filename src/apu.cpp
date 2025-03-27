#include "apu.h"
#include "ram.h"
#include <array>
#include <iomanip>
#include <iostream>
APU::APU(RamBus &ram) : _ram(ram)
{
    _next_cycle = SAMPLE_PERIOD;
    _sweep_cycle = 0;
    _timer_cycle = 0;
    _buffer_index = 0;
    _total_cycle = 0;
    fillTables();
    init_sdl();
}

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
}

auto APU::fillTables() -> void
{
    _duty_cycles[0] = 12.5;
    _duty_cycles[1] = 25.0;
    _duty_cycles[2] = 50.0;
    _duty_cycles[3] = 75.0;
    for (int i = 1; i < 2048; i++)
    {
        for (int cycle = 0; cycle < 4; cycle++)
        {
            float period_value = i;
            float frequency = ((CPU_FREQ / 4.0) / 8.0) / (2048.0 - period_value);
            float period_len = SAMPLERATE / frequency;
            float rise_len = (_duty_cycles[cycle] * period_len) / 100.0;
            _step_rise[i][cycle] = rise_len;
            _step_max[i][cycle] = period_len;
        }
    }
}

auto APU::step(uint32_t cycles_count) -> void
{
    _total_cycle += cycles_count;
    _next_cycle -= cycles_count;
    _sweep_cycle -= cycles_count;
    _timer_cycle -= cycles_count;
    for (int i = 0; i < 4; i++)
        _channels[i].enveloppe_timer_count -= cycles_count;
    if (_next_cycle <= 0)
    {
        for (int i = 0; i < 4; i++)
            _channels[i].value = 0;
        if (_ram[NR52] & 0x80) // audio on
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
    update_timer();
}

auto APU::flush() -> void
{
    if (!SDL_PutAudioStreamData(_audio_stream, _buffer.data(), _buffer_index * sizeof(int16_t)))
        throw std::runtime_error(std::string("SDL_PutAudioStreamData : ") + SDL_GetError());
    /*FILE *f = fopen("sound", "ab");
    fwrite(_buffer.data(), _buffer_index * sizeof(int16_t), 1, f);
    fclose(f);*/
    _buffer_index = 0;
}

auto APU::process_ch1() -> void
{
    int freq = _channels[0].freq;
    int sweep_pace = _ram[NR12] & 0x7;
    int duty = _ram[NR11] >> 6;
    int sweep_time = (_ram[NR10] >> 4) & 0x7;
    int step = _ram[NR10] & 0x7;
    int16_t value = 0;
    bool turn_off = false;

    if (sweep_time > 0 && _sweep_cycle <= 0)
    {
        if (_ram[NR10] & 0x08)
            freq = freq - (freq / (1 << step));
        else
            freq = freq + (freq / (1 << step));
        if (freq > 0x7FF)
            turn_off = true;
        _sweep_cycle += (CPU_FREQ / 128) * sweep_time;
        // std::cout << _channels[0].freq << " " << freq << " " << step << " " << (int)(_ram[NR10] & 0x08) << " "
        //           << sweep_time << " " << _sweep_cycle << " " << _total_cycle << std::endl;
    }
    if (sweep_pace > 0 && _channels[0].enveloppe_timer_count <= 0)
    {
        if (_ram[NR12] & 0x08)
            _channels[0].volume++;
        else
            _channels[0].volume--;
        if (_channels[0].volume < 0x0)
            _channels[0].volume = 0x0;
        if (_channels[0].volume > 0xF)
            _channels[0].volume = 0xF;
        _channels[0].enveloppe_timer_count += _channels[0].enveloppe_timer;
        // std::cout << _channels[0].volume << " " << volume << " " << (int)(_ram[NR12] & 0x08) << " "
        //           << _channels[0].enveloppe_timer << " " << _channels[0].enveloppe_timer_count << " " << _total_cycle
        //           << std::endl;
    }
    if (_channels[0].step > _step_max[freq][duty])
        _channels[0].step = 0;
    if (_channels[0].step > _step_rise[freq][duty])
        value = _channels[0].volume;
    if (turn_off)
        _ram.write_register(NR52, _ram[NR52] & 0xFE);
    _channels[0].step++;
    _channels[0].value = value;
    _channels[0].freq = freq;
}

auto APU::process_ch2() -> void
{
    int freq = _channels[1].freq;
    int sweep_pace = _ram[NR22] & 0x7;
    int duty = _ram[NR21] >> 6;
    int16_t value = 0;

    if (sweep_pace > 0 && _channels[1].enveloppe_timer_count <= 0)
    {
        if (_ram[NR22] & 0x08)
            _channels[1].volume++;
        else
            _channels[1].volume--;
        if (_channels[1].volume < 0x0)
            _channels[1].volume = 0x0;
        if (_channels[1].volume > 0xF)
            _channels[1].volume = 0xF;
        _channels[1].enveloppe_timer_count += _channels[1].enveloppe_timer;
    }
    if (_channels[1].step > _step_max[freq][duty])
        _channels[1].step = 0;
    if (_channels[1].step > _step_rise[freq][duty])
        value = _channels[1].volume;
    _channels[1].step++;
    _channels[1].value = value;
}

auto APU::process_ch3() -> void
{
    int freq = _channels[2].freq;
    int interval = 0;
    int index = 0;
    int16_t value = 0;

    if (freq > 0)
    {
        interval = (SAMPLERATE / freq) / SAMPLES;
        if (interval == 0)
            interval = 1;
        index = (_channels[2].step / interval) % SAMPLES;
        if ((index % 2) == 1)
            value = (_ram[WAVE_RAM + (index / 2)] >> 4) & 0xF;
        else
            value = _ram[WAVE_RAM + (index / 2)] & 0xF;
        // std::cout << "CH3 " << freq << " " << interval << " " << index << " " << _channels[2].volume << " "
        //           << _total_cycle << std::endl;
    }
    value >>= _channels[2].volume;
    _channels[2].step++;
    _channels[2].value = value;
}

auto APU::process_ch4() -> void
{
    int freq = _channels[3].freq;
    int sweep_pace = _ram[NR42] & 0x7;
    int feedback;
    int16_t value = 0;

    if (sweep_pace > 0 && _channels[3].enveloppe_timer_count < 0)
    {
        if (_ram[NR42] & 0x08)
            _channels[3].volume++;
        else
            _channels[3].volume--;
        if (_channels[3].volume < 0x0)
            _channels[3].volume = 0x0;
        if (_channels[3].volume > 0xF)
            _channels[3].volume = 0xF;
        _channels[3].enveloppe_timer_count += _channels[3].enveloppe_timer;
    }
    if (freq != 0 && _channels[3].step % freq == 0)
    {
        feedback = (_lfsr ^ (_lfsr >> 1)) & 1;
        _lfsr = (_lfsr >> 1) | (feedback << 15);
        if (_ram[NR43] & 0x08)
            _lfsr = (_lfsr & 0xFF7F) | (feedback << 7);
    }
    if ((_lfsr & 0x1) == 0)
        value = _channels[3].volume;
    _channels[3].step++;
    _channels[3].value = value;
}

auto APU::trigger_ch1() -> void
{
    int sweep_pace = _ram[NR12] & 0x7;
    int sweep_time = (_ram[NR10] >> 4) & 0x7;
    int period = ((_ram[NR14] & 0x7) << 8) | _ram[NR13];

    _ram.write_register(NR52, _ram[NR52] | 0x1);
    _channels[0].enveloppe_timer = (CPU_FREQ / 64) * sweep_pace;
    _channels[0].enveloppe_timer_count = _channels[0].enveloppe_timer;
    if (_ram[NR14] & 0x40)
        _channels[0].length_timer = _ram[NR11] & 0x3F;
    else
        _channels[0].length_timer = 255;
    _channels[0].freq = period;
    _channels[0].volume = _ram[NR12] >> 4;
    _channels[0].step = 0;
    _sweep_cycle = (CPU_FREQ / 128) * sweep_time;
    std::cout << "TRIG1 " << (int)_ram[NR10] << " " << (int)_ram[NR11] << " " << (int)_ram[NR12] << " "
              << (int)_ram[NR13] << " " << (int)_ram[NR14] << std::endl;
}

auto APU::trigger_ch2() -> void
{
    int sweep_pace = _ram[NR22] & 0x7;
    int period = ((_ram[NR24] & 0x7) << 8) | _ram[NR23];

    _ram.write_register(NR52, _ram[NR52] | 0x2);
    _channels[1].enveloppe_timer = (CPU_FREQ / 64) * sweep_pace;
    _channels[1].enveloppe_timer_count = _channels[1].enveloppe_timer;
    if (_ram[NR24] & 0x40)
        _channels[1].length_timer = _ram[NR21] & 0x3F;
    else
        _channels[1].length_timer = 255;
    _channels[1].freq = period;
    _channels[1].volume = _ram[NR22] >> 4;
    _channels[1].step = 0;
    std::cout << "TRIG2 " << (int)_ram[NR21] << " " << (int)_ram[NR22] << " " << (int)_ram[NR23] << " "
              << (int)_ram[NR24] << std::endl;
}

auto APU::trigger_ch3() -> void
{
    int sweep_pace = _ram[NR31];
    int period;

    _ram.write_register(NR52, _ram[NR52] | 0x4);
    if (_ram[NR34] & 0x40)
        _channels[2].length_timer = _ram[NR31];
    else
        _channels[2].length_timer = 512;
    period = ((_ram[NR34] & 0x7) << 8) | _ram[NR33];
    _channels[2].freq = 65536 / (2048 - period);
    _channels[2].step = 0;
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
    //    std::cout << "TRIG3 " << (int)_ram[NR31] << " " << (int)_ram[NR32] << " " << (int)_ram[NR33] << " "
    //              << (int)_ram[NR34] << std::endl;
}

auto APU::trigger_ch4() -> void
{
    int clock_shift = _ram[NR43] >> 4;
    int divider = _ram[NR43] & 0x7;
    int sweep_pace = _ram[NR42] & 0x7;
    int volume = (_ram[NR42] >> 4);

    if (divider == 0)
        divider = 1;
    _ram.write_register(NR52, _ram[NR52] | 0x8);
    _channels[3].enveloppe_timer = (CPU_FREQ / 64) * sweep_pace;
    _channels[3].enveloppe_timer_count = _channels[3].enveloppe_timer;
    if (_ram[NR44] & 0x40)
        _channels[3].length_timer = _ram[NR41] & 0x3F;
    else
        _channels[3].length_timer = 512;
    _channels[3].freq = SAMPLERATE / (262144 / (divider * (1 << clock_shift)));
    _channels[3].volume = _ram[NR22] >> 4;
    _channels[3].step = 0;
    if (_channels[3].freq == 0)
        _channels[3].freq = 1;
    _lfsr = 0xFFFF;
    std::cout << "TRIG4 " << (int)_ram[NR41] << " " << (int)_ram[NR42] << " " << (int)_ram[NR43] << " "
              << (int)_ram[NR44] << std::endl;
}

auto APU::update_timer() -> void
{
    if (_timer_cycle <= 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            _channels[i].length_timer++;
            if ((i == 2 && _channels[i].length_timer == 256) || (i != 2 && _channels[i].length_timer == 64))
            {
                std::cout << "DIS " << (int)i << std::endl;
                _ram.write_register(NR52, _ram[NR52] & ~(1 << i));
            }
        }
        _timer_cycle += TIMER_PERIOD;
    }
}

auto APU::mixer() -> void
{
    int16_t value = 0;
    int panning = _ram[NR51];
    int master_volume = _ram[NR51];

    for (int i = 0; i < 2; i++)
    {
        value = 0;
        for (int j = 0; j < 4; ++j)
        {
            if ((panning >> (4 * i + j)) & 1)
                value += _channels[j].value;
        }
        value *= 1 + ((master_volume >> (4 * i)) & 0x7);
        _buffer[_buffer_index] = -(value * 16);
        _buffer_index = (_buffer_index + 1) % AUDIO_BUFFER_SIZE;
    }
}