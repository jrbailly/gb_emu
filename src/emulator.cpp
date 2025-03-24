#include "emulator.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

Emulator::Emulator(const Config &configuration)
    : mRAM(), mCPU(std::make_unique<CPU>(mRAM)), mLCD(std::make_unique<LCD>(mRAM)),
      mControllers(std::make_unique<Controllers>()), mTimer(std::make_unique<Timer>()),
      mAPU(std::make_unique<APU>(mRAM)), mCartridge(std::make_unique<Cartridge>()), mConfig(configuration)
{
}

Emulator::~Emulator()
{
}

void Emulator::init()
{
    mCartridge->read_rom(mConfig.mRomFile);
    mCartridge->load_rom(mRAM);
    mCartridge->init(mRAM);
    mAPU->init(mRAM);
    mControllers->init(mRAM);
    mLCD->init(mRAM);
    mTimer->init(mRAM);
    mRAM.load_ram(mConfig.mRomFile);
}

void Emulator::loop()
{
    uint32_t cycles = 0;
    uint32_t cycles_count = 0;
    uint16_t last_addr;
    SDL_Event event;

    std::thread worker([&]() {
        while (true)
        {
            mRAM.save_ram(mConfig.mRomFile);
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    });
    worker.detach();
    while (true)
    {
        auto start_time = std::chrono::high_resolution_clock::now();

        cycles = 0;
        cycles_count = 0;
        while (::SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                mRAM.save_ram(mConfig.mRomFile);
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
        mLCD->reset();
        mTimer->reset();
        while (cycles_count < 70224)
        {
            // mCPU->debug(cycles);
            cycles = mCPU->step();
            mLCD->step(cycles);
            mAPU->step(cycles);
            mTimer->step(mRAM, cycles);
            cycles_count += cycles;
        }
        mLCD->renderer();
        mAPU->flush();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        SDL_DelayPrecise(1000.0 * (16742 - elapsed));
        /*FILE *f = fopen("ram", "wb");
        fwrite(&(*mRAM.begin()), 65535, 1, f);
        fclose(f);*/
    }
}
