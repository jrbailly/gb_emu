#include "application.h"
#include "bizhawk_record.h"
#include "frontend_sdl.h"
#include "vbm_record.h"
#include <SDL3/SDL.h>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

/**
 * @brief Constructs an Application object with default state.
 */
Application::Application()
{
}

/**
 * @brief Initializes the application with the given configuration.
 *
 * Creates the frontend, emulator, and optionally the input record replay system.
 * Also sets the audio sample rate from the frontend.
 *
 * @param configuration Configuration holding the ROM file path, screen scale,
 *                      audio filter setting, and optional record file path.
 */
auto Application::init(const Config &configuration) -> void
{
    _config = configuration;
    _emulator = std::make_unique<Emulator>(configuration);
    _emulator->init();
    _frontend = std::make_unique<FrontendSDL>(configuration, _emulator->get_refresh_rate());
    _emulator->set_samplerate(_frontend->get_samplerate());
    if (!configuration._recordfile.empty())
    {
        if (configuration._recordfile.ends_with(".bk2"))
            _record = std::make_unique<BizHawkRecord>();
        else
            _record = std::make_unique<VbmRecord>();
        _record->parse_file(configuration._recordfile);
    }
}

/**
 * @brief Runs the main emulation loop until the user quits.
 *
 * Each iteration polls inputs, handles save/load requests, steps the emulator
 * by one frame, snapshots the emulator state into the circular save state buffer,
 * and submits audio and video output to the frontend.
 */
auto Application::main_loop() -> void
{
    uint8_t pad = 0;
    uint8_t button = 0;

    while (!_quit)
    {
        _quit = _frontend->get_input(pad, button);
        if (_record)
            _record->get_input(pad, button);
        if (_frontend->pop_save_request())
            save_state_json();
        if (_frontend->pop_load_request())
            load_state_json();
        if (!_quit)
        {
            _emulator->set_input(pad, button);
            _emulator->step_frame();
            _emulator->save_state(_save_states[_save_state_index]);
            _frontend->play_audio(_emulator->get_audio_buffer());
            _frontend->render(_save_states[_save_state_index]);
            _save_state_index = (_save_state_index + 1) % save_state_buffer_size;
            _frontend->delay();
        }
    }
}

/**
 * @brief Serializes the most recent save state to a JSON file.
 *
 * Reads the last snapshot from the circular save state buffer and writes
 * each module's StateMap to a JSON file named after the loaded ROM.
 * StateValue variants are serialized by type: array, float, unsigned int, or int.
 *
 * @throws std::runtime_error if the output file cannot be opened.
 */
auto Application::save_state_json() -> void
{
    std::size_t last = (_save_state_index - 1 + save_state_buffer_size) % save_state_buffer_size;
    const SaveState &state = _save_states[last];
    nlohmann::json json_state;

    for (auto &[module_name, module_state] : state)
        for (auto &[key, val] : module_state)
            json_state[module_name][key] = std::visit([](auto &&v) -> nlohmann::json { return v; }, val);

    std::string filename = _config._romfile + ".json";
    std::ofstream file(filename);
    if (!file.is_open())
        throw std::runtime_error(std::format("cannot open file : {}", filename));
    file << json_state;
}

/**
 * @brief Deserializes a save state from a JSON file and restores the emulator.
 *
 * Reads the JSON file named after the loaded ROM, reconstructs each module's
 * StateMap into the current save state buffer slot, then calls the emulator's
 * load_state to apply it. JSON values are mapped back to the correct StateValue
 * variant type: array, float, unsigned int, or int.
 *
 * @throws std::runtime_error if the input file cannot be opened.
 */
auto Application::load_state_json() -> void
{
    std::string filename = _config._romfile + ".json";
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error(std::format("cannot open file : {}", filename));

    nlohmann::json json_state;
    file >> json_state;

    SaveState &state = _save_states[_save_state_index];
    for (auto &[module_name, module_state] : json_state.items())
        for (auto &[key, val] : module_state.items())
        {
            if (val.is_array())
                state[module_name][key] = val.get<std::vector<uint8_t>>();
            else if (val.is_number_float())
                state[module_name][key] = val.get<float>();
            else if (val.is_number_integer())
                state[module_name][key] = val.get<int>();
        }

    _emulator->load_state(state);
}
