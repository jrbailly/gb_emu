#ifndef _FRONTEND_QT_H_
#define _FRONTEND_QT_H_

#include "apu.h"
#include "config.h"
#include "framebuffer.h"
#include "highpass_filter.h"
#include "ifrontend.h"
#include "lcd.h"
#include <QAudioFormat>
#include <QAudioSink>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QMap>
#include <QQmlApplicationEngine>
#include <QThread>
#include <QTranslator>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

class FrontendQt : public QObject, public IFrontend
{
    Q_OBJECT
    Q_PROPERTY(Framebuffer *frame READ frame NOTIFY frameChanged)
    Q_PROPERTY(int screenWidth READ screenWidth NOTIFY resolutionChanged)
    Q_PROPERTY(int screenHeight READ screenHeight NOTIFY resolutionChanged)
    Q_PROPERTY(int screenScale READ screenScale NOTIFY resolutionChanged)
  public:
    FrontendQt(const Config &config, float refresh_rate);
    ~FrontendQt() override;
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
    auto eventFilter(QObject *watched, QEvent *event) -> bool override;
    auto frame() -> Framebuffer *
    {
        return &_framebuffer;
    }
    auto screenWidth() const -> int
    {
        return screen_width * _scale;
    }
    auto screenHeight() const -> int
    {
        return screen_height * _scale;
    }
    auto screenScale() const -> int
    {
        return _scale;
    }
  signals:

    auto frameChanged() -> void;
    auto resolutionChanged() -> void;

  public slots:
    auto load_rom(const QString &filename) -> void;
    auto set_scale(int scale) -> void;
    auto save_state() -> void;
    auto load_state() -> void;

  private:
    auto init_qt() -> void;
    auto init_translations() -> void;
    auto init_audio() -> void;
    auto init_controllers() -> void;

  private:
    int _argc{0};
    const Config &_config;
    int _scale{1};
    QGuiApplication _app;
    QQmlApplicationEngine _engine;
    std::unique_ptr<QTranslator> _translator;
    std::unique_ptr<QAudioSink> _audio_sink;
    QElapsedTimer _elapsed_timer;
    qint64 _next_frame;
    qint64 _increment_frame;
    uint32_t _samplerate{default_samplerate};
    QIODevice *_audio_device{nullptr};
    std::vector<int16_t> _audio_buffer;
    std::array<HighpassFilter, channels> _filter;
    std::atomic<uint8_t> _dpads{0xFF};
    std::atomic<uint8_t> _buttons{0xFF};
    std::atomic<bool> _save_requested{false};
    std::atomic<bool> _load_requested{false};
    std::atomic<bool> _rom_requested{false};
    std::string _rom_path;
    QMap<int, int> _dpads_binding_keys;
    QMap<int, int> _buttons_binding_keys;
    Framebuffer _framebuffer;
};

#endif
