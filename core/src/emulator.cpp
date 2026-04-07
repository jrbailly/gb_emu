#include "emulator.h"
#include <algorithm>
#include <chrono>

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
}

/**
 * @brief Runs one frame of emulation.
 *
 * Steps until the LCD signals end of VBlank (LY 153→0), or until 70224 cycles
 * have elapsed (full frame duration when the PPU is off).
 */
auto Emulator::step_frame() -> void
{
    int32_t max_frame_cycles = max_lines * cycles_per_line;
    uint32_t cycles = 0;
    bool vblank = false;

    _apu->flush();
    while (_cycles_count < max_frame_cycles && !vblank)
    {
        cycles = _cpu->step();
        _apu->step(cycles);
        _timer->step(_ram, cycles);
        vblank = _lcd->step(cycles);
        _cycles_count += cycles;
    }
    _cycles_count -= max_frame_cycles;
}

/**
 * @brief Returns the real Game Boy refresh rate in Hz.
 *
 * Computed from the CPU frequency and the exact number of cycles per frame
 * (154 lines × 456 cycles = 70224 cycles/frame), giving ≈59.7275 Hz.
 */
auto Emulator::get_refresh_rate() const -> float
{
    return static_cast<float>(cpu_freq) / static_cast<float>(max_lines * cycles_per_line);
}

auto Emulator::set_input(int pad, int button) -> void
{
    _controllers->set_input(pad, button);
}

auto Emulator::set_samplerate(float samplerate) -> void
{
    _apu->set_samplerate(samplerate);
}

/**
 * @brief Saves the current emulator state into the provided state map.
 *
 * Each module writes its state into its corresponding entry in the map.
 * An entry is only filled if the key is already present in the map,
 * allowing the caller to select which modules to snapshot.
 *
 * @param state_map Map of module name to StateMap, populated by the caller with the desired keys.
 */
auto Emulator::save_state(SaveState &state_map) -> void
{
    _ram.save_state(state_map["ram"]);
    _cpu->save_state(state_map["cpu"]);
    _apu->save_state(state_map["apu"]);
    _lcd->save_state(state_map["lcd"]);
    _timer->save_state(state_map["timer"]);
    _controllers->save_state(state_map["controllers"]);
}

auto Emulator::load_state(SaveState &state_map) -> void
{
    if (state_map.contains("ram"))
        _ram.load_state(state_map["ram"]);
    if (state_map.contains("cpu"))
        _cpu->load_state(state_map["cpu"]);
    if (state_map.contains("apu"))
        _apu->load_state(state_map["apu"]);
    if (state_map.contains("lcd"))
        _lcd->load_state(state_map["lcd"]);
    if (state_map.contains("timer"))
        _timer->load_state(state_map["timer"]);
    if (state_map.contains("controllers"))
        _controllers->load_state(state_map["controllers"]);
}
