#include "application.h"
#include "frontend_sdl.h"
#include "vbm_record.h"
#include <SDL3/SDL.h>
#include <format>

Application::Application()
{
}

auto Application::init(const Config &configuration) -> void
{
    _frontend = std::make_unique<FrontendSDL>(configuration);
    _emulator = std::make_unique<Emulator>(configuration);
    _emulator->init();
    if (!configuration._recordfile.empty())
    {
        _record = std::make_unique<VbmRecord>();
        _record->parse_file(configuration._recordfile);
    }
}

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
            _emulator->save_state();
        if (_frontend->pop_load_request())
            _emulator->load_state();
        if (!_quit)
        {
            _emulator->set_input(pad, button);
            _emulator->step_frame(_frontend->get_refresh_rate());
            _frontend->play_audio(_emulator->get_audio_buffer());
            _frontend->render(_emulator->get_frame_buffer());
            _frontend->delay();
        }
    }
}
