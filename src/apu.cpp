#include "apu.h"
#include "ram.h"
#include <array>
#include <iomanip>
#include <iostream>
APU::APU(MBC1 &ram) : mRAM(ram)
{
    mTotalCycle = 0;
    mNextCycle = SAMPLE_PERIOD;
    mSweepCycle = 0;
    mTimerCycle = 0;
    mBufferIndex = 0;
    fillTables();
    init_sdl();
}

void APU::init_sdl()
{
    SDL_AudioSpec spec;

    spec.format = SDL_AUDIO_S16;
    spec.channels = CHANNELS;
    spec.freq = SAMPLERATE;
    mAudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!mAudioStream)
        throw std::runtime_error(std::string("SDL_OpenAudioDeviceStream : ") + SDL_GetError());
    if (!SDL_ResumeAudioStreamDevice(mAudioStream))
        throw std::runtime_error(std::string("SDL_ResumeAudioStreamDevice : ") + SDL_GetError());
}

void APU::init(MBC1 &ram)
{
    ram.RegisterCallback(Register::NR14, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
        if (val & 0x80)
            trigger_ch1();
    });
    ram.RegisterCallback(Register::NR24, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
        if (val & 0x80)
            trigger_ch2();
    });
    ram.RegisterCallback(Register::NR34, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
        if (val & 0x80)
            trigger_ch3();
    });
    ram.RegisterCallback(Register::NR44, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
        if (val & 0x80)
            trigger_ch4();
    });
    ram.RegisterCallback(Register::NR30, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
        if ((val & 0x80) == 0)
            ram.write(NR52, ram[NR52] & ~(1 << 3));
    });
}

void APU::fillTables()
{
    mDutyCycles[0] = 12.5;
    mDutyCycles[1] = 25.0;
    mDutyCycles[2] = 50.0;
    mDutyCycles[3] = 75.0;
    for (int i = 1; i < 2048; i++)
    {
        for (int cycle = 0; cycle < 4; cycle++)
        {
            float period_value = i;
            float frequency = ((CPU_FREQ / 4.0) / 8.0) / (2048.0 - period_value);
            float period_len = SAMPLERATE / frequency;
            float rise_len = (mDutyCycles[cycle] * period_len) / 100.0;
            mStepRise[i][cycle] = rise_len;
            mStepMax[i][cycle] = period_len;
        }
    }
}

void APU::step(uint32_t cycles_count)
{
    if (mTotalCycle >= mNextCycle)
    {
        if (mRAM[NR52] & 0x80) // audio on
        {
            if (mRAM[NR52] & 0x1)
                process_ch1();
            if (mRAM[NR52] & 0x2)
                process_ch2();
            if (mRAM[NR52] & 0x4)
                process_ch3();
            if (mRAM[NR52] & 0x8)
                process_ch4();
        }
        mixer();
        mNextCycle += SAMPLE_PERIOD;
    }
    update_timer();
    mTotalCycle += cycles_count;
    mSweepCycle -= cycles_count;
    for (int i = 0; i < 4; i++)
    {
        mChannels[i].value = 0;
        mChannels[i].enveloppe_timer_count -= cycles_count;
    }
}

void APU::flush()
{
    if (!SDL_PutAudioStreamData(mAudioStream, mBuffer.data(), mBufferIndex * sizeof(int16_t)))
        throw std::runtime_error(std::string("SDL_PutAudioStreamData : ") + SDL_GetError());
    /*FILE *f = fopen("sound", "ab");
    fwrite(mBuffer.data(), mBufferIndex * sizeof(int16_t), 1, f);
    fclose(f);*/
    mBufferIndex = 0;
    mNextCycle = SAMPLE_PERIOD;
    mTimerCycle = TIMER_PERIOD;
    mTotalCycle = 0;
}

void APU::process_ch1()
{
    int freq = mChannels[0].freq;
    int sweep_pace = mRAM[NR12] & 0x7;
    int duty = mRAM[NR11] >> 6;
    int sweep_time = (mRAM[NR10] >> 4) & 0x7;
    int step = mRAM[NR10] & 0x7;
    int16_t value = 0;
    bool turn_off = false;

    if (sweep_time > 0 && mSweepCycle < 0)
    {
        if (mRAM[NR10] & 0x08)
            freq = freq - (freq / (1 << step));
        else
            freq = freq + (freq / (1 << step));
        if (freq > 0x7FF)
            turn_off = true;
        mSweepCycle = (CPU_FREQ / 128) * sweep_time;
    }
    if (sweep_pace > 0 && mChannels[0].enveloppe_timer_count < 0)
    {
        if (mRAM[NR12] & 0x08)
            mChannels[0].volume++;
        else
            mChannels[0].volume--;
        if (mChannels[0].volume < 0x0)
            mChannels[0].volume = 0x0;
        if (mChannels[0].volume > 0xF)
            mChannels[0].volume = 0xF;
        mChannels[0].enveloppe_timer_count = mChannels[0].enveloppe_timer;
    }
    if (mChannels[0].step > mStepMax[freq][duty])
        mChannels[0].step = 0;
    if (mChannels[0].step > mStepRise[freq][duty])
        value = mChannels[0].volume;
    if (turn_off)
        mRAM.write(NR52, mRAM[NR52] & 0xFE);
    mChannels[0].step++;
    mChannels[0].value = value;
    mChannels[0].freq = freq;
}

void APU::process_ch2()
{
    int freq = mChannels[1].freq;
    int sweep_pace = mRAM[NR22] & 0x7;
    int duty = mRAM[NR21] >> 6;
    int16_t value = 0;

    if (sweep_pace > 0 && mChannels[1].enveloppe_timer < 0)
    {
        if (mRAM[NR12] & 0x08)
            mChannels[1].volume++;
        else
            mChannels[1].volume--;
        if (mChannels[1].volume < 0x0)
            mChannels[1].volume = 0x0;
        if (mChannels[1].volume > 0xF)
            mChannels[1].volume = 0xF;
    }
    if (mChannels[1].step > mStepMax[freq][duty])
        mChannels[1].step = 0;
    if (mChannels[1].step > mStepRise[freq][duty])
        value = mChannels[1].volume;
    mChannels[1].step++;
    mChannels[1].value = value;
}

void APU::process_ch3()
{
    int freq = mChannels[2].freq;
    int interval = 0;
    int index = 0;
    int16_t value = 0;

    if (freq > 0)
    {
        interval = (SAMPLERATE / freq) / SAMPLES;
        if (interval == 0)
            interval = 1;
        index = (mChannels[2].step / interval) % SAMPLES;
        if ((index % 2) == 1)
            value = (mRAM[WAVE_RAM + (index / 2)] >> 4) & 0xF;
        else
            value = mRAM[WAVE_RAM + (index / 2)] & 0xF;
    }
    value >>= mChannels[2].volume;
    mChannels[2].step++;
    mChannels[2].value = value;
}

void APU::process_ch4()
{
    int freq = mChannels[3].freq;
    int sweep_pace = mRAM[NR42] & 0x7;
    int feedback;
    int16_t value = 0;

    if (sweep_pace > 0 && mChannels[3].enveloppe_timer < 0)
    {
        if (mRAM[NR42] & 0x08)
            mChannels[3].volume++;
        else
            mChannels[3].volume--;
        if (mChannels[3].volume < 0x0)
            mChannels[3].volume = 0x0;
        if (mChannels[3].volume > 0xF)
            mChannels[3].volume = 0xF;
    }
    if (mChannels[3].step % freq == 0)
    {
        feedback = (lfsr ^ (lfsr >> 1)) & 1;
        lfsr = (lfsr >> 1) | (feedback << 15);
        if (mRAM[NR43] & 0x08)
            lfsr = (lfsr & 0xFF7F) | (feedback << 7);
    }
    if ((lfsr & 0x1) == 0)
        value = mChannels[3].volume;
    mChannels[3].step++;
    mChannels[3].value = value;
}

void APU::trigger_ch1()
{
    int sweep_pace = mRAM[NR12] & 0x7;
    int sweep_time = (mRAM[NR10] >> 4) & 0x7;

    mRAM.write(NR52, mRAM[NR52] | 0x1);
    mChannels[0].enveloppe_timer = (CPU_FREQ / 64) * sweep_pace;
    mChannels[0].enveloppe_timer_count = mChannels[0].enveloppe_timer;
    if (mRAM[NR14] & 0x40)
        mChannels[0].length_timer = mRAM[NR11] & 0x3F;
    else
        mChannels[0].length_timer = 255;
    mChannels[0].freq = ((mRAM[NR14] & 0x7) << 8) | mRAM[NR13];
    mChannels[0].volume = mRAM[NR12] >> 4;
    mChannels[0].step = 0;
    mSweepCycle = (CPU_FREQ / 128) * sweep_time;
}

void APU::trigger_ch2()
{
    int sweep_pace = mRAM[NR22] & 0x7;

    mRAM.write(NR52, mRAM[NR52] | 0x2);
    mChannels[1].enveloppe_timer = (CPU_FREQ / 64) * sweep_pace;
    mChannels[1].enveloppe_timer_count = mChannels[0].enveloppe_timer;
    if (mRAM[NR24] & 0x40)
        mChannels[1].length_timer = mRAM[NR21] & 0x3F;
    else
        mChannels[1].length_timer = 255;
    mChannels[1].freq = ((mRAM[NR24] & 0x7) << 8) | mRAM[NR23];
    mChannels[1].volume = mRAM[NR22] >> 4;
    mChannels[1].step = 0;
}

void APU::trigger_ch3()
{
    int sweep_pace = mRAM[NR31];

    mRAM.write(NR52, mRAM[NR52] | 0x4);
    if (mRAM[NR34] & 0x40)
        mChannels[2].length_timer = mRAM[NR31];
    else
        mChannels[2].length_timer = 512;
    mChannels[2].freq = 65536 / (2048 - ((mRAM[NR34] & 0x7) << 8) | mRAM[NR33]);
    mChannels[2].step = 0;
    switch ((mRAM[NR32] >> 5) & 0x3)
    {
    case 0:
        mChannels[2].volume = 4;
        break;
    case 1:
        mChannels[2].volume = 0;
        break;
    case 2:
        mChannels[2].volume = 1;
        break;
    case 3:
        mChannels[2].volume = 2;
        break;
    };
}

void APU::trigger_ch4()
{
    int clock_shift = mRAM[NR43] >> 4;
    int divider = mRAM[NR43] & 0x7;
    int sweep_pace = mRAM[NR42] & 0x7;
    int volume = (mRAM[NR42] >> 4);

    if (divider == 0)
        divider = 1;
    mRAM.write(NR52, mRAM[NR52] | 0x8);
    mChannels[3].enveloppe_timer = (CPU_FREQ / 64) * sweep_pace;
    mChannels[3].enveloppe_timer_count = mChannels[3].enveloppe_timer;
    if (mRAM[NR44] & 0x40)
        mChannels[3].length_timer = mRAM[NR41] & 0x3F;
    else
        mChannels[3].length_timer = 512;
    mChannels[3].freq = SAMPLERATE / (262144 / (divider * (1 << clock_shift)));
    mChannels[3].volume = mRAM[NR22] >> 4;
    mChannels[3].step = 0;
    if (mChannels[3].freq == 0)
        mChannels[3].freq = 1;
    lfsr = 0xFFFF;
}

void APU::update_timer()
{
    if (mTotalCycle > mTimerCycle)
    {
        for (int i = 0; i < 4; ++i)
        {
            mChannels[i].length_timer++;
            if ((i == 2 && mChannels[i].length_timer == 256) || (i != 2 && mChannels[i].length_timer == 64))
                mRAM.write(NR52, mRAM[NR52] & ~(1 << i));
        }
        mTimerCycle += TIMER_PERIOD;
    }
}

void APU::mixer()
{
    int16_t value = 0;
    int panning = mRAM[NR51];
    int master_volume = mRAM[NR51];

    for (int i = 0; i < 2; i++)
    {
        value = 0;
        for (int j = 0; j < 4; ++j)
        {
            if ((panning >> (4 * i + j)) & 1)
                value += mChannels[j].value;
        }
        value *= 1 + ((master_volume >> (4 * i)) & 0x7);
        mBuffer[mBufferIndex] = -(value * 16);
        mBufferIndex = (mBufferIndex + 1) % AUDIO_BUFFER_SIZE;
    }
}