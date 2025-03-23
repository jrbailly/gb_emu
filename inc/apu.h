#ifndef _APU_H_
#define _APU_H_
#include "cpu.h"
#include <SDL3/SDL.h>

static constexpr int SAMPLERATE = 44100;
static constexpr int SAMPLE_PERIOD = CPU_FREQ / SAMPLERATE;
static constexpr int TIMER_PERIOD = CPU_FREQ / 256;
static constexpr int FREQUENCIES = 2048;
static constexpr int DUTY_CYCLES = 4;
static constexpr int CHANNELS = 2;
static constexpr int AUDIO_BUFFER_SIZE = 65536;
static constexpr int SAMPLES = 32;

struct Channel
{
    int step;
    int length_timer;
    int enveloppe_timer;
    int enveloppe_timer_count;
    int freq;
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
        NR51 = 0xFF25,
        NR52 = 0xFF26,
        WAVE_RAM = 0xFF30,
    };
    APU(MBC1 &ram);
    void step(uint32_t cycles_count, uint16_t last_addr);
    void flush();
    void callback(int additional_amount, int total_amount);

  private:
    void init_sdl();
    void fillTables();
    void process_ch1();
    void process_ch2();
    void process_ch3();
    void process_ch4();
    void trigger_ch1();
    void trigger_ch2();
    void trigger_ch3();
    void trigger_ch4();
    void update_timer();
    void mixer();

  private:
    MBC1 &mRAM;
    int mTotalCycle;
    int mNextCycle;
    int mSweepCycle;
    int mTimerCycle;
    int mBufferIndex;
    float mDutyCycles[DUTY_CYCLES];
    int mStepRise[FREQUENCIES][DUTY_CYCLES];
    int mStepMax[FREQUENCIES][DUTY_CYCLES];
    uint16_t lfsr;
    SDL_AudioStream *mAudioStream;
    Channel mChannels[4];
    std::array<int16_t, AUDIO_BUFFER_SIZE * CHANNELS> mBuffer;
};

#endif