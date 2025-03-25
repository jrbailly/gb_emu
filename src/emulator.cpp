#include "emulator.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

/**
 * @brief Construct a new Emulator object
 *
 * @param configuration Configuration
 */
Emulator::Emulator(const Config &configuration)
    : _ram(), _cpu(std::make_unique<CPU>(_ram)), _lcd(std::make_unique<LCD>(_ram)),
      _controllers(std::make_unique<Controllers>()), _timer(std::make_unique<Timer>()),
      _apu(std::make_unique<APU>(_ram)), _cartridge(std::make_unique<Cartridge>()), _config(configuration)
{
}

/**
 * @brief Init emulator.
 * - Load rom
 * - Register write callback in peripherals.
 * - Load last saved memory
 *
 */
auto Emulator::init() -> void
{
    _cartridge->read_rom(_config.mRomFile);
    _cartridge->load_rom(_ram);
    _cartridge->init(_ram);
    _apu->init(_ram);
    _controllers->init(_ram);
    _lcd->init(_ram);
    _timer->init(_ram);
    _ram.load_ram(_config.mRomFile);
}

/**
 * @brief Mainloop
 *
 */
auto Emulator::loop() -> void
{
    uint32_t cycles = 0;
    uint32_t cycles_count = 0;
    uint16_t last_addr;
    SDL_Event event;

    std::thread worker([&]() {
        while (true)
        {
            _ram.save_ram(_config.mRomFile);
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
                _ram.save_ram(_config.mRomFile);
                return;
                break;
            case SDL_EVENT_KEY_DOWN:
                _controllers->setInput(event.key);
                break;
            case SDL_EVENT_KEY_UP:
                _controllers->setInput(event.key);
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                _controllers->setInput(event.gbutton);
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                _controllers->setInput(event.gbutton);
                break;
            default:
                break;
            }
        }
        _lcd->reset();
        while (cycles_count < frame_cycle_count)
        {
            //_cpu->debug(cycles_count);
            cycles = _cpu->step();
            _lcd->step(cycles);
            _apu->step(cycles);
            _timer->step(_ram, cycles);
            cycles_count += cycles;
        }
        _lcd->renderer();
        _apu->flush();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        SDL_DelayPrecise(1000.0 * (frame_duration - elapsed));
        FILE *f = fopen("ram", "wb");
        fwrite(_ram.data(), 1, 65535, f);
        fclose(f);
    }
}
