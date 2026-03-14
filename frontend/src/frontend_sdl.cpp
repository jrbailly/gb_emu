#include "frontend_sdl.h"
#include "controllers.h"
#include <format>
#include <vector>

/**
 * @brief Constructs the SDL frontend, opens the audio device and initialises input bindings.
 * @param config Application configuration.
 */
FrontendSDL::FrontendSDL(const Config &config)
    : _active_filter(config._audio_filter), _audio_stream(nullptr), _dpads(0xF), _buttons(0xF), _save_requested(false),
      _load_requested(false)
{
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
        throw std::runtime_error(std::format("SDL_Init : {}", SDL_GetError()));
    init_audio();
    init_graphics();
    init_controllers();
}

/**
 * @brief Opens the SDL audio device and starts playback.
 */
auto FrontendSDL::init_audio() -> void
{
    SDL_AudioSpec spec;

    spec.format = SDL_AUDIO_S16;
    spec.channels = CHANNELS;
    spec.freq = (int)SAMPLERATE;
    _audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!_audio_stream)
        throw std::runtime_error(std::format("SDL_OpenAudioDeviceStream : {}", SDL_GetError()));
    if (!SDL_ResumeAudioStreamDevice(_audio_stream))
        throw std::runtime_error(std::format("SDL_ResumeAudioStreamDevice : {}", SDL_GetError()));
}

/**
 * @brief Initializes the graphical subsystem. Not implemented yet.
 */
auto FrontendSDL::init_graphics() -> void
{
}

/**
 * @brief Initializes the controllers subsystem.
 */
auto FrontendSDL::init_controllers() -> void
{
    _dpads_binding[SDLK_UP] = Controllers::UP;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_UP] = Controllers::UP;
    _dpads_binding[SDLK_DOWN] = Controllers::DOWN;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = Controllers::DOWN;
    _dpads_binding[SDLK_LEFT] = Controllers::LEFT;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = Controllers::LEFT;
    _dpads_binding[SDLK_RIGHT] = Controllers::RIGHT;
    _dpads_binding[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = Controllers::RIGHT;
    _buttons_binding[SDLK_RETURN] = Controllers::START;
    _buttons_binding[SDL_GAMEPAD_BUTTON_START] = Controllers::START;
    _buttons_binding[SDLK_BACKSPACE] = Controllers::SELECT;
    _buttons_binding[SDL_GAMEPAD_BUTTON_GUIDE] = Controllers::SELECT;
    _buttons_binding[SDLK_LCTRL] = Controllers::A;
    _buttons_binding[SDL_GAMEPAD_BUTTON_SOUTH] = Controllers::A;
    _buttons_binding[SDLK_LALT] = Controllers::B;
    _buttons_binding[SDL_GAMEPAD_BUTTON_EAST] = Controllers::B;
}

/**
 * @brief Destroys the SDL frontend and closes the audio stream.
 */
FrontendSDL::~FrontendSDL()
{
    if (_audio_stream)
    {
        SDL_DestroyAudioStream(_audio_stream);
        SDL_Quit();
        _audio_stream = nullptr;
    }
}

/**
 * @brief Polls SDL events, updates the input state and returns whether a quit was requested.
 *
 * Key and gamepad button events update the internal d-pad and button bitmasks.
 * F1 and F2 set the save/load request flags consumed by @ref pop_save_request
 * and @ref pop_load_request.
 *
 * @param pad    Output: current d-pad bitmask.
 * @param button Output: current button bitmask.
 * @return true if a quit event was received.
 */
auto FrontendSDL::get_input(int &pad, int &button) -> bool
{
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            return true;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_F1)
                _save_requested = true;
            if (event.key.key == SDLK_F2)
                _load_requested = true;
            if (_dpads_binding.count(event.key.key))
                _dpads &= ~_dpads_binding[event.key.key];
            if (_buttons_binding.count(event.key.key))
                _buttons &= ~_buttons_binding[event.key.key];
            break;
        case SDL_EVENT_KEY_UP:
            if (_dpads_binding.count(event.key.key))
                _dpads |= _dpads_binding[event.key.key];
            if (_buttons_binding.count(event.key.key))
                _buttons |= _buttons_binding[event.key.key];
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            if (_dpads_binding.count(event.gbutton.button))
                _dpads &= ~_dpads_binding[event.gbutton.button];
            if (_buttons_binding.count(event.gbutton.button))
                _buttons &= ~_buttons_binding[event.gbutton.button];
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (_dpads_binding.count(event.gbutton.button))
                _dpads |= _dpads_binding[event.gbutton.button];
            if (_buttons_binding.count(event.gbutton.button))
                _buttons |= _buttons_binding[event.gbutton.button];
            break;
        default:
            break;
        }
    }
    pad = _dpads;
    button = _buttons;
    return false;
}

/**
 * @brief Consume and return the pending save-state request (F1).
 * @return true once if F1 was pressed since the last call.
 */
auto FrontendSDL::pop_save_request() -> bool
{
    bool requested = _save_requested;
    _save_requested = false;
    return requested;
}

/**
 * @brief Consume and return the pending load-state request (F2).
 * @return true once if F2 was pressed since the last call.
 */
auto FrontendSDL::pop_load_request() -> bool
{
    bool requested = _load_requested;
    _load_requested = false;
    return requested;
}

/**
 * @brief Waits for the given number of microseconds using SDL_DelayPrecise.
 * @param us Number of microseconds to wait.
 */
auto FrontendSDL::delay(int us) -> void
{
    SDL_DelayPrecise(static_cast<double>(us) * 1000.0);
}

/**
 * @brief Applies the high-pass filter if active, then sends the buffer to the SDL audio stream.
 * @param buffer View of the audio samples produced by the APU.
 */
auto FrontendSDL::play_audio(std::span<const int16_t> buffer) -> void
{
    std::vector<int16_t> copy(buffer.begin(), buffer.end());
    if (_active_filter)
        for (int channel = 0; channel < CHANNELS; ++channel)
            _filter[channel].filter(std::span<int16_t>(copy), channel, CHANNELS);
    if (!SDL_PutAudioStreamData(_audio_stream, copy.data(), copy.size() * sizeof(int16_t)))
        throw std::runtime_error(std::format("SDL_PutAudioStreamData : {}", SDL_GetError()));
}

/**
 * @brief Not implemented in this step.
 */
auto FrontendSDL::display(const uint8_t *frame_buffer, int width, int height) -> void
{
    (void)frame_buffer;
    (void)width;
    (void)height;
}
