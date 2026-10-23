#include "audio_sdl.h"
#include <algorithm>
#include <format>
#include <stdexcept>

AudioSDL::AudioSDL() = default;

AudioSDL::~AudioSDL()
{
}

/**
 * @brief Opens the SDL audio stream and starts playback.
 *
 * The stream is opened at @ref samplerate with signed 16-bit samples; SDL
 * converts to the device format if the hardware uses another rate.
 */
auto AudioSDL::init_audio() -> void
{
    SDL_AudioSpec spec;

    spec.format = SDL_AUDIO_S16;
    spec.channels = channels;
    spec.freq = static_cast<int>(_samplerate);
    _audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!_audio_stream)
        throw std::runtime_error(std::format("SDL_OpenAudioDeviceStream : {}", SDL_GetError()));
    if (!SDL_ResumeAudioStreamDevice(_audio_stream))
        throw std::runtime_error(std::format("SDL_ResumeAudioStreamDevice : {}", SDL_GetError()));
}

/**
 * @brief Releases the SDL audio stream.
 *
 * Must be called before SDL_Quit() when the owner shuts the SDL subsystems down,
 * since the stream cannot be destroyed once SDL has been uninitialised.
 */
auto AudioSDL::shutdown() -> void
{
    if (_audio_stream)
    {
        SDL_DestroyAudioStream(_audio_stream);
        _audio_stream = nullptr;
    }
}

/**
 * @brief Applies the high-pass filter if active, then sends the buffer to the SDL audio stream.
 * @param buffer View of the audio samples produced by the APU.
 * @param audio_filter True when the high-pass filter must be applied.
 */
auto AudioSDL::play_audio(std::span<const int16_t> buffer, bool audio_filter) -> void
{
    if (buffer.empty() || _audio_stream == nullptr)
        return;

    if (_audio_buffer.size() != buffer.size())
        _audio_buffer.resize(buffer.size());
    std::copy(buffer.begin(), buffer.end(), _audio_buffer.begin());

    if (audio_filter)
        for (int channel = 0; channel < channels; ++channel)
            _filter[channel].filter(std::span<int16_t>(_audio_buffer), channel, channels);

    if (!SDL_PutAudioStreamData(_audio_stream, _audio_buffer.data(),
                                static_cast<int>(_audio_buffer.size() * sizeof(int16_t))))
        throw std::runtime_error(std::format("SDL_PutAudioStreamData : {}", SDL_GetError()));
}

/**
 * @brief Returns the audio sample rate used by the audio stream.
 * @return Sample rate in Hz.
 */
auto AudioSDL::samplerate() const -> uint32_t
{
    return _samplerate;
}
