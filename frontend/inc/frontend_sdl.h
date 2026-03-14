#ifndef _FRONTEND_SDL_H_
#define _FRONTEND_SDL_H_

#include "apu.h"
#include "config.h"
#include "highpass_filter.h"
#include "ifrontend.h"
#include <SDL3/SDL.h>

class Frontend_SDL : public IFrontend
{
  public:
    Frontend_SDL(const Config &config);
    ~Frontend_SDL() override;
    auto poll_inputs() -> bool override;
    auto play_audio(std::span<const int16_t> buffer) -> void override;
    auto display(const uint8_t *frame_buffer, int width, int height) -> void override;

  private:
    bool _active_filter;
    SDL_AudioStream *_audio_stream;
    HighpassFilter _filter[CHANNELS];
};

#endif
