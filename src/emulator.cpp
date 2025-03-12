#include "emulator.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

Emulator::Emulator(const Config &configuration)
    : mRAM(configuration.mRomFile), mCPU(std::make_unique<CPU>(mRAM)), mLCD(std::make_unique<LCD>(mRAM)),
      mControllers(std::make_unique<Controllers>(mRAM)), mConfig(configuration)
{
}

Emulator::~Emulator()
{
}

void Emulator::init()
{
    mRAM.init();
}

void Emulator::loop()
{
    uint32_t cycles = 0;

    auto start_time = std::chrono::high_resolution_clock::now();
    while (true)
    {
        // mCPU->debug(cycles);
        cycles += mCPU->step();
        mLCD->step(cycles);
        mControllers->step();
        mRAM.resetLastWrite();
        if (cycles >= 70224)
        {
            cycles = 0;
            mLCD->renderer();
            mLCD->reset();
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            std::this_thread::sleep_for(std::chrono::microseconds(16742 - elapsed));
            start_time = std::chrono::high_resolution_clock::now();
            FILE *f = fopen("ram", "wb");
            fwrite(mRAM.data(), 65535, 1, f);
            fclose(f);

            SDL_Event event;
            while (::SDL_PollEvent(&event) != 0)
            {
                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                    return;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    mControllers->setInput(event.key);
                    break;
                case SDL_EVENT_KEY_UP:
                    mControllers->setInput(event.key);
                    break;
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                    mControllers->setInput(event.gbutton);
                    break;
                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                    mControllers->setInput(event.gbutton);
                    break;
                default:
                    break;
                }
            }
        }
    }
}
