#include "emulator.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

Emulator::Emulator(const Config &configuration)
    : mRAM(configuration.mRomFile), mCPU(std::make_unique<CPU>(mRAM)), mLCD(std::make_unique<LCD>(mRAM)),
      mControllers(std::make_unique<Controllers>()), mConfig(configuration)
{
}

Emulator::~Emulator()
{
}

void Emulator::init()
{
}

void Emulator::loop()
{
    uint32_t cycles = 0;

    auto start_time = std::chrono::high_resolution_clock::now();
    while (true)
    {
        cycles += mCPU->step();
        mLCD->step(cycles);
        if (cycles >= 65564)
            mLCD->render();
        if (cycles >= 70224)
        {
            cycles = 0;
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            std::this_thread::sleep_for(std::chrono::microseconds(16742 - elapsed));
            start_time = std::chrono::high_resolution_clock::now();
            FILE *f = fopen("ram", "wb");
            fwrite(mRAM.data(), 65535, 1, f);
            fclose(f);
        }
    }
}
