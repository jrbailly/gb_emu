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
      _load_requested(false), _scale(config._screen_scale)
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
    spec.channels = channels;
    spec.freq = (int)samplerate;
    _audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!_audio_stream)
        throw std::runtime_error(std::format("SDL_OpenAudioDeviceStream : {}", SDL_GetError()));
    if (!SDL_ResumeAudioStreamDevice(_audio_stream))
        throw std::runtime_error(std::format("SDL_ResumeAudioStreamDevice : {}", SDL_GetError()));
}

/**
 * @brief Initializes the graphical subsystem: window, renderer, and streaming texture.
 */
auto FrontendSDL::init_graphics() -> void
{
    _window = SDL_CreateWindow("", _scale * screen_width, _scale * screen_height, 0);
    if (!_window)
        throw std::runtime_error(std::format("SDL_CreateWindow : {}", SDL_GetError()));
    _renderer = SDL_CreateRenderer(_window, NULL);
    if (!_renderer)
        throw std::runtime_error(std::format("SDL_CreateRenderer : {}", SDL_GetError()));
    _texture_viewer = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                        screen_width + texture_padding, screen_height);
    if (!_texture_viewer)
        throw std::runtime_error(std::format("SDL_CreateTexture : {}", SDL_GetError()));
    if (!SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255))
        throw std::runtime_error(std::format("SDL_SetRenderDrawColor : {}", SDL_GetError()));
    if (!SDL_SetTextureScaleMode(_texture_viewer, SDL_SCALEMODE_NEAREST))
        throw std::runtime_error(std::format("SDL_SetTextureScaleMode : {}", SDL_GetError()));
}

/**
 * @brief Initializes the controllers subsystem.
 */
auto FrontendSDL::init_controllers() -> void
{
    _dpads_binding_keys[SDLK_UP] = Controllers::UP;
    _dpads_binding_keys[SDLK_DOWN] = Controllers::DOWN;
    _dpads_binding_keys[SDLK_LEFT] = Controllers::LEFT;
    _dpads_binding_keys[SDLK_RIGHT] = Controllers::RIGHT;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_UP] = Controllers::UP;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = Controllers::DOWN;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = Controllers::LEFT;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = Controllers::RIGHT;
    _buttons_binding_keys[SDLK_RETURN] = Controllers::START;
    _buttons_binding_keys[SDLK_BACKSPACE] = Controllers::SELECT;
    _buttons_binding_keys[SDLK_LCTRL] = Controllers::A;
    _buttons_binding_keys[SDLK_LALT] = Controllers::B;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_START] = Controllers::START;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_GUIDE] = Controllers::SELECT;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_SOUTH] = Controllers::A;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_EAST] = Controllers::B;
}

/**
 * @brief Destroys the SDL frontend and closes the audio stream.
 */
FrontendSDL::~FrontendSDL()
{
    if (_texture_viewer)
        SDL_DestroyTexture(_texture_viewer);
    if (_renderer)
        SDL_DestroyRenderer(_renderer);
    if (_window)
        SDL_DestroyWindow(_window);
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
auto FrontendSDL::get_input(uint8_t &pad, uint8_t &button) -> bool
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
            if (_dpads_binding_keys.count(event.key.key))
                _dpads &= ~_dpads_binding_keys[event.key.key];
            if (_buttons_binding_keys.count(event.key.key))
                _buttons &= ~_buttons_binding_keys[event.key.key];
            break;
        case SDL_EVENT_KEY_UP:
            if (_dpads_binding_keys.count(event.key.key))
                _dpads |= _dpads_binding_keys[event.key.key];
            if (_buttons_binding_keys.count(event.key.key))
                _buttons |= _buttons_binding_keys[event.key.key];
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            if (_dpads_binding_gamepad.count(event.gbutton.button))
                _dpads &= ~_dpads_binding_gamepad[event.gbutton.button];
            if (_buttons_binding_gamepad.count(event.gbutton.button))
                _buttons &= ~_buttons_binding_gamepad[event.gbutton.button];
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (_dpads_binding_gamepad.count(event.gbutton.button))
                _dpads |= static_cast<uint8_t>(_dpads_binding_gamepad[event.gbutton.button]);
            if (_buttons_binding_gamepad.count(event.gbutton.button))
                _buttons |= static_cast<uint8_t>(_buttons_binding_gamepad[event.gbutton.button]);
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
auto FrontendSDL::delay(int32_t us) -> void
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
        for (int channel = 0; channel < channels; ++channel)
            _filter[channel].filter(std::span<int16_t>(copy), channel, channels);
    if (!SDL_PutAudioStreamData(_audio_stream, copy.data(), copy.size() * sizeof(int16_t)))
        throw std::runtime_error(std::format("SDL_PutAudioStreamData : {}", SDL_GetError()));
}

/**
 * @brief Uploads the framebuffer into the SDL texture and presents it.
 * @param frame_buffer View of the LCD framebuffer (width = screen_width + texture_padding, height = screen_height).
 */
auto FrontendSDL::render(std::span<const uint32_t> frame_buffer) -> void
{
    void *pixels = nullptr;
    int pitch = 0;

    if (!SDL_LockTexture(_texture_viewer, nullptr, &pixels, &pitch))
        throw std::runtime_error(std::format("SDL_LockTexture : {}", SDL_GetError()));
    std::copy(frame_buffer.begin(), frame_buffer.end(), static_cast<uint32_t *>(pixels));
    SDL_UnlockTexture(_texture_viewer);
    SDL_FRect src(texture_offset, 0, screen_width, screen_height);
    if (!SDL_SetRenderScale(_renderer, _scale, _scale))
        throw std::runtime_error(std::format("SDL_SetRenderScale : {}", SDL_GetError()));
    if (!SDL_RenderTexture(_renderer, _texture_viewer, &src, NULL))
        throw std::runtime_error(std::format("SDL_RenderTexture : {}", SDL_GetError()));
    if (!SDL_FlushRenderer(_renderer))
        throw std::runtime_error(std::format("SDL_FlushRenderer : {}", SDL_GetError()));
    if (!SDL_RenderPresent(_renderer))
        throw std::runtime_error(std::format("SDL_RenderPresent : {}", SDL_GetError()));
    if (!SDL_RenderClear(_renderer))
        throw std::runtime_error(std::format("SDL_RenderClear : {}", SDL_GetError()));
}
