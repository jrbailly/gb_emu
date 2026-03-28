#ifndef _FRONTEND_SDL_H_
#define _FRONTEND_SDL_H_

#include "apu.h"
#include "config.h"
#include "highpass_filter.h"
#include "ifrontend.h"
#include "lcd.h"
#include <SDL3/SDL.h>
#include <map>

class FrontendSDL : public IFrontend
{
  public:
    FrontendSDL(const Config &config);
    ~FrontendSDL();
    auto get_input(int &pad, int &button) -> bool override;
    auto pop_save_request() -> bool override;
    auto pop_load_request() -> bool override;
    auto delay(int us) -> void override;
    auto play_audio(std::span<const int16_t> buffer) -> void override;
    auto render(std::span<const uint32_t> frame_buffer) -> void override;

  private:
    auto init_audio() -> void;
    auto init_graphics() -> void;
    auto init_controllers() -> void;

  private:
    bool _active_filter;
    SDL_AudioStream *_audio_stream;
    SDL_Window *_window = nullptr;
    SDL_Renderer *_renderer = nullptr;
    SDL_Texture *_texture_viewer = nullptr;
    HighpassFilter _filter[channels];
    int _dpads;
    int _buttons;
    std::map<int, int> _dpads_binding_keys;
    std::map<int, int> _dpads_binding_gamepad;
    std::map<int, int> _buttons_binding_keys;
    std::map<int, int> _buttons_binding_gamepad;
    bool _save_requested;
    bool _load_requested;
    int _scale;
};

#endif
