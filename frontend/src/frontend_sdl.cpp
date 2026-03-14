#include "frontend_sdl.h"
#include <format>
#include <vector>

/**
 * @brief Constructs the SDL frontend and opens the audio device.
 * @param config Application configuration.
 */
Frontend_SDL::Frontend_SDL(const Config &config) : _active_filter(config._audio_filter)
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
 * @brief Destroys the SDL frontend and closes the audio stream.
 */
Frontend_SDL::~Frontend_SDL()
{
    SDL_DestroyAudioStream(_audio_stream);
}

/**
 * @brief Not implemented in this step.
 */
auto Frontend_SDL::poll_inputs() -> bool
{
    return false;
}

/**
 * @brief Applies the high-pass filter if active, then sends the buffer to the SDL audio stream.
 * @param buffer View of the audio samples produced by the APU.
 */
auto Frontend_SDL::play_audio(std::span<const int16_t> buffer) -> void
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
auto Frontend_SDL::display(const uint8_t *frame_buffer, int width, int height) -> void
{
    (void)frame_buffer;
    (void)width;
    (void)height;
}
