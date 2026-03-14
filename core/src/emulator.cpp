#include "emulator.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

/**
 * @brief Construct a new Emulator object
 *
 * @param configuration Configuration
 */
Emulator::Emulator(const Config &configuration)
    : _ram(), _apu(std::make_unique<APU>(_ram)), _cartridge(std::make_unique<Cartridge>()),
      _controllers(std::make_unique<Controllers>(_ram)), _cpu(std::make_unique<CPU>(_ram)),
      _lcd(std::make_unique<LCD>(_ram)), _timer(std::make_unique<Timer>()), _config(configuration), _cycles_count(0)
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
    _apu->init(_ram);
    _controllers->init(_ram);
    _lcd->init(_ram);
    _timer->init(_ram);
    _lcd->set_scale(_config._screen_scale);
}

/**
 * @brief Runs one frame of emulation.
 * @return Number of microseconds the caller should wait to maintain 60 FPS.
 *         Returns 0 if the frame took longer than the target duration.
 */
auto Emulator::step_frame() -> int
{
    uint32_t cycles = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    while (_cycles_count < frame_cycle_count)
    {
        cycles = _cpu->step();
        _apu->step(cycles);
        _lcd->step(cycles);
        _timer->step(_ram, cycles);
        _cycles_count += cycles;
    }
    _apu->flush();
    _cycles_count -= frame_cycle_count;
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    return (frame_duration - elapsed);
}

auto Emulator::set_input(int pad, int button) -> void
{
    _controllers->set_input(pad, button);
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
