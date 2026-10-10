#include "application.h"
#include "bizhawk_record.h"
#include "vbm_record.h"
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#ifdef FRONTEND_QT
#include "frontend_qt.h"
#else
#include "frontend_sdl.h"
#include <SDL3/SDL.h>
#endif

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
 * The initial ROM is loaded before the emulator thread is started, so that any
 * ROM validation error surfaces on the calling thread.
 * Also sets the audio sample rate from the frontend.
 *
 * @param configuration Configuration holding the ROM file path, screen scale,
 *                      audio filter setting, and optional record file path.
 */
auto Application::init(const Config &configuration) -> void
{
    _config = configuration;
    _emulator = std::make_unique<Emulator>();
    _emulator->init();
    _emulator->load_rom(_config._romfile);
#ifdef FRONTEND_QT
    _frontend = std::make_unique<FrontendQt>(_config, _emulator->get_refresh_rate());
#else
    _frontend = std::make_unique<FrontendSDL>(_config, _emulator->get_refresh_rate());
#endif
    _emulator->set_samplerate(_frontend->get_samplerate());
    if (!_config._recordfile.empty())
    {
        if (_config._recordfile.ends_with(".bk2"))
            _record = std::make_unique<BizHawkRecord>();
        else
            _record = std::make_unique<VbmRecord>();
        _record->parse_file(_config._recordfile);
    }
    _emulator_thread = std::jthread(&Application::main_loop, this);
}

/**
 * @brief Runs the main emulation loop until the user quits.
 *
 * Each iteration polls inputs, handles the frontend requests (save state, load state,
 * ROM load), steps the emulator by one frame, snapshots the emulator state into the
 * circular save state buffer, and submits audio and video output to the frontend.
 */
auto Application::main_loop() -> void
{
    uint8_t pad = 0;
    uint8_t button = 0;
    std::string romfile;

    _frontend->init();
    while (!_quit)
    {
        // Process inputs
        _frontend->get_input(pad, button);
        if (_record)
            _record->get_input(pad, button);
        _emulator->set_input(pad, button);

        // Process requests
        if (_frontend->pop_save_request())
            save_state_json();
        if (_frontend->pop_load_request())
            load_state_json();
        if (_frontend->pop_rom_request(romfile))
        {
            _emulator->load_rom(romfile);
            _config._romfile = romfile;
        }

        // Frame simulation
        _emulator->step_frame();

        // Rendering
        _emulator->save_state(_save_states[_save_state_index]);
        _frontend->play_audio(_emulator->get_audio_buffer());
        _frontend->render(_emulator->get_graphic_buffer());
        _save_state_index = (_save_state_index + 1) % save_state_buffer_size;

        _frontend->delay();
    }
}

/**
 * @brief Blocks on the frontend event loop until quit, then stops the emulator thread.
 *
 * Runs the frontend's blocking event loop on the calling thread. Once it returns
 * (quit requested), the emulator thread started by @ref init is signalled to stop
 * and joined.
 */
auto Application::process_events() -> void
{
    _frontend->process_events();
    _quit = true;
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
