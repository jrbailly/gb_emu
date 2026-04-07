#include "timer.h"
#include "cpu.h"

/**
 * @brief Construct a new Timer object
 *
 */
Timer::Timer() : _next_cycle_div(0), _next_cycle_tima(0), _cycle_tima(0)
{
    _clocks_cycles[0] = cpu_freq / 4096;
    _clocks_cycles[1] = cpu_freq / 262144;
    _clocks_cycles[2] = cpu_freq / 65536;
    _clocks_cycles[3] = cpu_freq / 16384;
}

/**
 * @brief Saves the timer state into the provided StateMap.
 *
 * Stores the remaining cycles before the next DIV increment, the remaining
 * cycles before the next TIMA increment, and the current TIMA clock period.
 *
 * @param state StateMap to write the timer state into.
 */
auto Timer::save_state(StateMap &state) -> void
{
    state["div_counter"] = (int)_next_cycle_div;
    state["tima_counter"] = (int)_next_cycle_tima;
    state["tima_period"] = (int)_cycle_tima;
}

/**
 * @brief Restores the timer state from the provided StateMap.
 *
 * Reads back the DIV counter, TIMA counter, and TIMA clock period
 * that were previously saved.
 *
 * @param state StateMap containing the previously saved timer state.
 */
auto Timer::load_state(const StateMap &state) -> void
{
    _next_cycle_div = std::get<int>(state.at("div_counter"));
    _next_cycle_tima = std::get<int>(state.at("tima_counter"));
    _cycle_tima = std::get<int>(state.at("tima_period"));
}

/**
 * @brief Initializes the timer with a RAM bus for register callbacks.
 *
 * @param ram Reference to the RAM bus.
 **/
auto Timer::init(RamBus &ram) -> void
{
    ram.register_callback(Register::TAC, [this](RamBus &, int, unsigned char val) {
        int clock = val & 0x3;

        if (val & 0x4)
        {
            _cycle_tima = _clocks_cycles[clock];
            _next_cycle_tima = _cycle_tima;
        }
        else
        {
            _cycle_tima = 0;
            _next_cycle_tima = -1;
        }
    });
    ram.register_callback(Register::DIV, [](RamBus &ram, int, unsigned char) { ram.write_register(Register::DIV, 0); });
}

/**
 * @brief Increase register DIV and TIMA based on elapsed cycles.
 *        DIV increments every 16384 CPU cycles, while TIMA increments
 *        based on the clock frequency selected in TAC.
 * @param ram Reference to the RAM bus for register access
 * @param cycles_count Number of CPU cycles elapsed since last call
 */
auto Timer::step(RamBus &ram, uint32_t cycles_count) -> void
{
    _next_cycle_div -= cycles_count;
    _next_cycle_tima -= cycles_count;
    if (_next_cycle_div <= 0)
    {
        unsigned char div = ram[Register::DIV] + 1;

        ram.write_register(Register::DIV, div);
        _next_cycle_div += cycles_per_div_increment;
    }
    while (_next_cycle_tima <= 0 && _cycle_tima > 0)
    {
        unsigned char tima = ram[Register::TIMA];

        if (tima == 0xFF)
        {
            uint8_t interrupt = ram[CPU::Register::IF];
            unsigned char tma = ram[Register::TMA];

            tima = tma;
            ram.write_register(CPU::Register::IF, interrupt | CPU::IFFlag::TIMER);
        }
        else
            tima++;
        ram.write_register(Register::TIMA, tima);
        _next_cycle_tima += _cycle_tima;
    }
}
