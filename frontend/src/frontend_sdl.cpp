#include "frontend_sdl.h"
#include "controllers.h"
#include "emulator.h"
#include <cstring>
#include <format>
#include <iostream>

/**
 * @brief Constructs the SDL frontend, opens the audio device and initialises input bindings.
 * @param config       Application configuration.
 * @param refresh_rate Real Game Boy refresh rate in Hz (e.g. ≈59.7275).
 */
FrontendSDL::FrontendSDL(const Config &config, float refresh_rate)
    : _active_filter(config._audio_filter), _dpads(0xF), _buttons(0xF), _save_requested(false), _load_requested(false),
      _scale(config._screen_scale)
{
    _increment_frame = SDL_GetPerformanceFrequency() / refresh_rate;
    _next_frame = SDL_GetPerformanceCounter() + _increment_frame;
}

/**
 * @brief Destroys the SDL frontend and shuts SDL down.
 */
FrontendSDL::~FrontendSDL()
{
    if (_texture_viewer)
        SDL_DestroyTexture(_texture_viewer);
    if (_renderer)
        SDL_DestroyRenderer(_renderer);
    if (_window)
        SDL_DestroyWindow(_window);
    _audio.shutdown();
    SDL_Quit();
}

/**
 * @brief Initialize SDL , opens the audio device and initialises input bindings.
 */
auto FrontendSDL::init() -> void
{
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
        throw std::runtime_error(std::format("SDL_Init : {}", SDL_GetError()));
    _audio.init_audio();
    init_graphics();
    init_controllers();
    _initialized = true;
}

/**
 * @brief Reports an error message on the standard error stream.
 * @param message Message to report.
 */
auto FrontendSDL::log(std::string_view message) -> void
{
    std::cerr << message << std::endl;
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
    _texture_viewer = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screen_width,
                                        screen_height);
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
    _dpads_binding_keys[SDL_SCANCODE_W] = Controllers::UP;
    _dpads_binding_keys[SDL_SCANCODE_S] = Controllers::DOWN;
    _dpads_binding_keys[SDL_SCANCODE_A] = Controllers::LEFT;
    _dpads_binding_keys[SDL_SCANCODE_D] = Controllers::RIGHT;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_UP] = Controllers::UP;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = Controllers::DOWN;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = Controllers::LEFT;
    _dpads_binding_gamepad[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = Controllers::RIGHT;
    _buttons_binding_keys[SDL_SCANCODE_RETURN] = Controllers::START;
    _buttons_binding_keys[SDL_SCANCODE_BACKSPACE] = Controllers::SELECT;
    _buttons_binding_keys[SDL_SCANCODE_L] = Controllers::A;
    _buttons_binding_keys[SDL_SCANCODE_K] = Controllers::B;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_START] = Controllers::START;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_GUIDE] = Controllers::SELECT;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_SOUTH] = Controllers::A;
    _buttons_binding_gamepad[SDL_GAMEPAD_BUTTON_EAST] = Controllers::B;
}

/**
 * @brief Returns the current input state.
 *
 * Event decoding is performed by @ref process_events through @ref event_filter;
 * this method only exposes the resulting pad/button bitmasks to the application.
 *
 * @param pad    Output: current d-pad bitmask.
 * @param button Output: current button bitmask.
 */
auto FrontendSDL::get_input(uint8_t &pad, uint8_t &button) -> void
{
    pad = _dpads;
    button = _buttons;
}

/**
 * @brief Decodes a single SDL event, updating the input state and request flags.
 *
 * Key and gamepad button events update the internal d-pad and button bitmasks,
 * while other events may raise the save or load request flags consumed by
 * @ref pop_save_request and @ref pop_load_request. A quit event or the Escape
 * key ends the blocking event loop in @ref process_events.
 *
 * @param event Event to decode.
 * @return true if a quit event or the Escape key was received.
 */
auto FrontendSDL::event_filter(const SDL_Event &event) -> bool
{
    switch (event.type)
    {
    case SDL_EVENT_QUIT:
        return true;
    case SDL_EVENT_KEY_DOWN:
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
            return true;
        if (event.key.repeat)
            break;
        if (event.key.scancode == SDL_SCANCODE_F1)
            _save_requested = true;
        if (event.key.scancode == SDL_SCANCODE_F2)
            _load_requested = true;
        if (_dpads_binding_keys.count(event.key.scancode))
            _dpads &= static_cast<uint8_t>(~_dpads_binding_keys[event.key.scancode]);
        if (_buttons_binding_keys.count(event.key.scancode))
            _buttons &= static_cast<uint8_t>(~_buttons_binding_keys[event.key.scancode]);
        break;
    case SDL_EVENT_KEY_UP:
        if (_dpads_binding_keys.count(event.key.scancode))
            _dpads |= static_cast<uint8_t>(_dpads_binding_keys[event.key.scancode]);
        if (_buttons_binding_keys.count(event.key.scancode))
            _buttons |= static_cast<uint8_t>(_buttons_binding_keys[event.key.scancode]);
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (_dpads_binding_gamepad.count(event.gbutton.button))
            _dpads &= static_cast<uint8_t>(~_dpads_binding_gamepad[event.gbutton.button]);
        if (_buttons_binding_gamepad.count(event.gbutton.button))
            _buttons &= static_cast<uint8_t>(~_buttons_binding_gamepad[event.gbutton.button]);
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
    return false;
}

/**
 * @brief Runs the blocking SDL event loop until a quit event is received.
 *
 * Waits until @ref init has completed before entering the SDL event loop, then
 * waits for each event and feeds it to @ref event_filter, returning once a quit
 * event has been handled. SDL_WaitEvent sleeps while the queue is empty, so this
 * does not spin the CPU.
 */
auto FrontendSDL::process_events() -> void
{
    SDL_Event event;

    while (!_initialized)
        SDL_Delay(10);

    while (true)
    {
        SDL_WaitEvent(&event);
        if (event_filter(event))
            return;
    }
}

/**
 * @brief Consume and return the pending save-state request (F1).
 * @return true once if F1 was pressed since the last call.
 */
auto FrontendSDL::pop_save_request() -> bool
{
    return _save_requested.exchange(false);
}

/**
 * @brief Consume and return the pending load-state request (F2).
 * @return true once if F2 was pressed since the last call.
 */
auto FrontendSDL::pop_load_request() -> bool
{
    return _load_requested.exchange(false);
}

/**
 * @brief Consume and return the pending ROM load request.
 *
 * The SDL frontend offers no ROM selection interface, so no request can ever be
 * raised: this always reports that nothing is pending.
 *
 * @param filename Output: left untouched.
 * @return false, always.
 */
auto FrontendSDL::pop_rom_request(std::string & /*filename*/) -> bool
{
    return false;
}

/**
 * @brief Sleeps until the next frame deadline, then advances it by one frame period.
 */
auto FrontendSDL::delay() -> void
{
    if (_next_frame > SDL_GetPerformanceCounter())
    {
        auto wait = ((_next_frame - SDL_GetPerformanceCounter()) * 1000000000) / SDL_GetPerformanceFrequency();

        SDL_DelayPrecise(wait);
    }
    _next_frame += _increment_frame;
}

/**
 * @brief Forwards the APU samples to the SDL audio output.
 * @param buffer View of the audio samples produced by the APU.
 */
auto FrontendSDL::play_audio(std::span<const int16_t> buffer) -> void
{
    _audio.play_audio(buffer, _active_filter);
}

/**
 * @brief Returns the audio sample rate used by the audio output.
 * @return Sample rate in Hz.
 */
auto FrontendSDL::get_samplerate() -> uint32_t
{
    return _audio.samplerate();
}

/**
 * @brief Uploads the de-padded LCD frame into the SDL texture and presents it.
 * @param buffer RGBA8888 frame produced by Emulator::get_graphic_buffer().
 */
auto FrontendSDL::render(std::span<const uint8_t> buffer) -> void
{
    void *pixels = nullptr;
    int pitch = 0;

    if (!SDL_LockTexture(_texture_viewer, nullptr, &pixels, &pitch))
        throw std::runtime_error(std::format("SDL_LockTexture : {}", SDL_GetError()));
    std::copy(buffer.begin(), buffer.end(), static_cast<uint8_t *>(pixels));
    SDL_UnlockTexture(_texture_viewer);
    SDL_FRect src(0, 0, screen_width, screen_height);
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
