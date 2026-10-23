#ifndef _AUDIO_SDL_H_
#define _AUDIO_SDL_H_

#include "apu.h"
#include "highpass_filter.h"
#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

class AudioSDL
{
  public:
    AudioSDL();
    ~AudioSDL();
    auto play_audio(std::span<const int16_t> buffer, bool audio_filter) -> void;
    auto init_audio() -> void;
    auto shutdown() -> void;
    auto samplerate() const -> uint32_t;

  private:
    SDL_AudioStream *_audio_stream{nullptr};
    std::vector<int16_t> _audio_buffer;
    std::array<HighpassFilter, channels> _filter;
    uint32_t _samplerate{default_samplerate};
};

#endif
