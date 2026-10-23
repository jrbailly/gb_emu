#ifndef _AUDIO_QT_H_
#define _AUDIO_QT_H_

#include "apu.h"
#include "highpass_filter.h"
#include <QAudioSink>
#include <QIODevice>
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class AudioQt
{
  public:
    AudioQt();
    virtual ~AudioQt();
    auto play_audio(std::span<const int16_t> buffer, bool audio_filter) -> void;
    auto init_audio() -> void;
    auto samplerate() const -> uint32_t;

  private:
    std::unique_ptr<QAudioSink> _audio_sink;
    QIODevice *_audio_device{nullptr};
    std::vector<int16_t> _audio_buffer;
    std::array<HighpassFilter, channels> _filter;
    uint32_t _samplerate{default_samplerate};
};

#endif
