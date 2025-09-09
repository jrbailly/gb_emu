#include "emulator.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>

/**
 * @brief Construct a new Emulator object
 *
 * @param configuration Configuration
 */
Emulator::Emulator(const Config &configuration)
    : _ram(), _apu(std::make_unique<APU>(_ram)), _cartridge(std::make_unique<Cartridge>()),
      _controllers(std::make_unique<Controllers>(_ram)), _cpu(std::make_unique<CPU>(_ram)),
      _lcd(std::make_unique<LCD>(_ram)), _timer(std::make_unique<Timer>()), _config(configuration)
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
    _cartridge->read_rom(_config._romfile);
    _cartridge->load_rom(_ram);
    _cartridge->init(_ram);
    _apu->init(_ram, _config);
    _controllers->init(_ram, _config);
    _lcd->init(_ram);
    _timer->init(_ram);
    _lcd->set_scale(_config._screen_scale);
}

/**
 * @brief Mainloop
 *
 */
auto Emulator::loop() -> void
{
    uint32_t cycles = 0;
    uint32_t cycles_count = 0;
    uint32_t total_cycles = 0;

    while (true)
    {
        auto start_time = std::chrono::high_resolution_clock::now();

        cycles = 0;
        cycles_count = 0;
        if (process_sdl_events())
            return;
        while (cycles_count < frame_cycle_count)
        {
            //_cpu->debug(total_cycles);
            cycles = _cpu->step();
            _apu->step(cycles);
            _controllers->step(cycles);
            _lcd->step(cycles);
            _timer->step(_ram, cycles);
            cycles_count += cycles;
            total_cycles += cycles;
        }
        _apu->flush();
        cycles_count -= frame_cycle_count;
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        SDL_DelayPrecise(1000.0 * (frame_duration - elapsed));
    }
}

/**
 * @brief Process sdl events
 *
 */
auto Emulator::process_sdl_events() -> bool
{
    SDL_Event event;

    while (::SDL_PollEvent(&event) != 0)
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            return (true);
            break;
        case SDL_EVENT_KEY_DOWN:
            _controllers->setInput(event.key.key, event.key.down);
            if (event.key.key == SDLK_F1)
                save_state();
            if (event.key.key == SDLK_F2)
                load_state();
            break;
        case SDL_EVENT_KEY_UP:
            _controllers->setInput(event.key.key, event.key.down);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            _controllers->setInput(event.gbutton.button, event.gbutton.down);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            _controllers->setInput(event.gbutton.button, event.gbutton.down);
            break;
        default:
            break;
        }
    }
    return (false);
}

auto Emulator::save_state() -> void
{
    nlohmann::json state;
    std::string filename = _config._romfile + ".json";
    std::ofstream file(filename.data());

    if (!file.is_open())
        throw std::runtime_error(std::format("cannot open file : {}", filename));

    state["cpu"] = _cpu->get_registers();
    state["ram"] = _ram.getDatas();
    if (file)
        file << state;
}

auto Emulator::load_state() -> void
{
    nlohmann::json state;
    std::string filename = _config._romfile + ".json";
    std::ifstream file(filename.data());
    std::map<std::string, int> registers;
    int address = 0;

    if (!file.is_open())
        throw std::runtime_error(std::format("cannot open file : {}", filename));
    file >> state;
    for (auto &[key, value] : state["cpu"].items())
        registers[key] = value;
    _cpu->load_registers(registers);

    for (auto &value : state["ram"])
        _ram.write_register(address++, value.get<unsigned char>());

    _apu->load_state();
    _lcd->load_state();
}
