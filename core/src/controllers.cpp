#include "controllers.h"
#include "cpu.h"

/**
 * @brief Constructs a Controllers object and initializes input state.
 *
 * All D-pad and button bits are set to 0xF (released) by default.
 *
 * @param ram Reference to the RamBus used for interrupt signaling.
 */
Controllers::Controllers(RamBus &ram) : _ram(ram), _dpads(0xf), _buttons(0xf)
{
}

/**
 * @brief Saves the controller state into the provided StateMap.
 *
 * Stores the current D-pad and button values under the keys "pads" and "buttons".
 *
 * @param state StateMap to write the controller state into.
 */
auto Controllers::save_state(StateMap &state) -> void
{
    state["pads"] = _dpads;
    state["buttons"] = _buttons;
}

/**
 * @brief Restores the controller state from the provided StateMap.
 *
 * Reads the D-pad and button values from the keys "pads" and "buttons".
 *
 * @param state StateMap containing the previously saved controller state.
 */
auto Controllers::load_state(const StateMap &state) -> void
{
    _dpads = std::get<int>(state.at("pads"));
    _buttons = std::get<int>(state.at("buttons"));
}

/**
 * @brief Initializes the controller by registering a write callback on the JOYP register.
 *
 * The callback updates the JOYP register with the current D-pad or button state
 * depending on the selection bits written by the game.
 *
 * @param ram Reference to the RamBus used to register the callback.
 */
auto Controllers::init(RamBus &ram) -> void
{
    ram.register_callback(Register::JOYP, [this](RamBus &ram, int, unsigned char val) {
        uint8_t select = val & (ESelect::BUTTON | ESelect::DPAD);
        uint8_t low;

        if (select == 0x00)
            low = _buttons & _dpads;
        else if ((select & ESelect::BUTTON) == 0)
            low = _buttons;
        else if ((select & ESelect::DPAD) == 0)
            low = _dpads;
        else
            low = 0x0F;

        ram.write_register(Register::JOYP, 0xC0 | select | low);
    });
}

/**
 * @brief Updates the current D-pad and button input state.
 *
 * Triggers a joypad interrupt if the input has changed since the last call.
 *
 * @param pad  Bitmask of the current D-pad state.
 * @param button Bitmask of the current button state.
 */
auto Controllers::set_input(int pad, int button) -> void
{
    if (_dpads != pad || _buttons != button)
        _ram.write_register(CPU::Register::IF, _ram[CPU::Register::IF] | CPU::IFFlag::JOYPAD);
    _dpads = pad;
    _buttons = button;
}
