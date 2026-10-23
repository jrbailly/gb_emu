#ifndef _FRONTEND_SDL_H_
#define _FRONTEND_SDL_H_

#include "audio_sdl.h"
#include "config.h"
#include "ifrontend.h"
#include "lcd.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <map>

class FrontendSDL : public IFrontend
{
  public:
    FrontendSDL(const Config &config, float refresh_rate);
    virtual ~FrontendSDL();
    auto init() -> void override;
    auto get_input(uint8_t &pad, uint8_t &button) -> void override;
    auto process_events() -> void override;
    auto pop_save_request() -> bool override;
    auto pop_load_request() -> bool override;
    auto pop_rom_request(std::string &filename) -> bool override;
    auto delay() -> void override;
    auto play_audio(std::span<const int16_t> buffer) -> void override;
    auto render(std::span<const uint8_t> buffer) -> void override;
    auto get_samplerate() -> uint32_t override;
    auto log(std::string_view message) -> void override;

  private:
    auto init_graphics() -> void;
    auto init_controllers() -> void;
    auto event_filter(const SDL_Event &event) -> bool;

  private:
    bool _active_filter;
    std::atomic<bool> _initialized{false};
    AudioSDL _audio;
    SDL_Window *_window = nullptr;
    SDL_Renderer *_renderer = nullptr;
    SDL_Texture *_texture_viewer = nullptr;
    std::atomic<uint8_t> _dpads;
    std::atomic<uint8_t> _buttons;
    std::map<SDL_Scancode, int> _dpads_binding_keys;
    std::map<int, int> _dpads_binding_gamepad;
    std::map<SDL_Scancode, int> _buttons_binding_keys;
    std::map<int, int> _buttons_binding_gamepad;
    std::atomic<bool> _save_requested;
    std::atomic<bool> _load_requested;
    int _scale;
    uint64_t _next_frame;
    uint64_t _increment_frame;
};

#endif
