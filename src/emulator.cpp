#include "emulator.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

Emulator::Emulator(const Config &configuration)
    : mRAM(configuration.mRomFile), mCPU(std::make_unique<CPU>(mRAM)), mLCD(std::make_unique<LCD>(mRAM)),
      mControllers(std::make_unique<Controllers>(mRAM)), mTimer(std::make_unique<Timer>(mRAM)), mConfig(configuration)
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
    uint16_t last_addr;
    SDL_Event event;

    while (true)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
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
        cycles = 0;
        while (cycles < 70224)
        {
            // mCPU->debug(cycles);
            cycles += mCPU->step();
            last_addr = mRAM.lastWrite();
            mLCD->step(cycles, last_addr);
            mTimer->step(cycles, last_addr);
            mControllers->step(last_addr);
        }
        mLCD->renderer();
        mLCD->reset();
        mTimer->reset();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        std::this_thread::sleep_for(std::chrono::microseconds(16742 - elapsed));
        if (16742 - elapsed < 0)
            std::cout << "holly shit !" << std::endl;
        /*FILE *f = fopen("ram", "wb");
        fwrite(mRAM.data(), 65535, 1, f);
        fclose(f);*/
    }
}
