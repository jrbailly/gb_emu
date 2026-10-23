#include "audio_qt.h"
#include <algorithm>
#include <format>
#include <stdexcept>
#include <QMediaDevices>

AudioQt::AudioQt() = default;

AudioQt::~AudioQt()
{
    if (_audio_sink)
        _audio_sink->stop();
}

/**
 * @brief Applies the high-pass filter if active, then sends the buffer to the Qt audio sink.
 * @param buffer View of the audio samples produced by the APU.
 * @param audio_filter True when the high-pass filter must be applied.
 */
auto AudioQt::play_audio(std::span<const int16_t> buffer, bool audio_filter) -> void
{
    qint64 total = static_cast<qint64>(buffer.size()) * sizeof(int16_t);
    qint64 done = 0;
    qint64 writed = 0;

    if (buffer.empty() || _audio_device == nullptr)
        return;

    if (_audio_buffer.size() != buffer.size())
        _audio_buffer.resize(buffer.size());
    std::copy(buffer.begin(), buffer.end(), _audio_buffer.begin());

    if (audio_filter)
        for (int channel = 0; channel < channels; ++channel)
            _filter[channel].filter(std::span<int16_t>(_audio_buffer), channel, channels);

    while (done < total)
    {
        writed = _audio_device->write(reinterpret_cast<const char *>(_audio_buffer.data()) + done, total - done);
        done += writed;
        if (writed < 0)
            throw std::runtime_error(
                std::format("QIODevice write failed : {}", _audio_device->errorString().toStdString()));
    }
}

/**
 * @brief Returns the audio sample rate used by the audio sink.
 * @return Sample rate in Hz.
 */
auto AudioQt::samplerate() const -> uint32_t
{
    return _samplerate;
}

/**
 * @brief Opens the Qt audio sink and starts playback.
 *
 * The sample rate is taken from the preferred format of the default audio device
 * when one is available.
 */
auto AudioQt::init_audio() -> void
{
    QAudioFormat format;
    QAudioDevice device = QMediaDevices::defaultAudioOutput();

    if (!device.isNull())
        _samplerate = device.preferredFormat().sampleRate();

    format.setSampleRate(_samplerate);
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::Int16);

    _audio_sink = std::make_unique<QAudioSink>(format);
    _audio_sink->setVolume(1.0);
    _audio_device = _audio_sink->start();
    if (!_audio_device)
        throw std::runtime_error("QAudioSink::start: failed to open audio device");
}
