#include "frontend_qt.h"
#include "controllers.h"
#include "version.h"
#include <QDebug>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMediaDevices>
#include <QQmlContext>
#include <QThread>
#include <QUrl>
#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <vector>

/**
 * @brief Constructs the Qt frontend, loads the QML interface and initialises audio and input bindings.
 * @param config       Application configuration.
 * @param refresh_rate Real Game Boy refresh rate in Hz (e.g. ≈59.7275).
 */
FrontendQt::FrontendQt(const Config &config, float refresh_rate)
    : _config(config), _scale(config._screen_scale), _app(_argc, nullptr)
{
    _increment_frame = static_cast<qint64>(1000000000.0f / refresh_rate);
    _next_frame = _increment_frame;
    init_qt();
    init_audio();
    _controllers.init_controllers();
    connect(this, &FrontendQt::languageChanged, &_controllers, &ControllersQt::keyBindingsChanged);
}

/**
 * @brief Destroys the Qt frontend and stops the audio sink.
 */
FrontendQt::~FrontendQt()
{
    if (_audio_sink)
        _audio_sink->stop();
    _app.removeTranslator(_translator.get());
}

/**
 * @brief Initialize the application
 */
auto FrontendQt::init() -> void
{
    _elapsed_timer.start();
}

/**
 * @brief Returns the current input state.
 *
 * Event decoding is performed by the Qt event loop through @ref eventFilter;
 * this method only exposes the resulting pad/button bitmasks to the application.
 *
 * @param pad    Output: current d-pad bitmask.
 * @param button Output: current button bitmask.
 */
auto FrontendQt::get_input(uint8_t &pad, uint8_t &button) -> void
{
    pad = _controllers.get_dpads();
    button = _controllers.get_buttons();
}

/**
 * @brief Runs the blocking Qt event loop until the application exits.
 */
auto FrontendQt::process_events() -> void
{
    _app.exec();
}

/**
 * @brief Consume and return the pending save-state request (F1).
 * @return true once if F1 was pressed since the last call.
 */
auto FrontendQt::pop_save_request() -> bool
{
    return _save_requested.exchange(false);
}

/**
 * @brief Consume and return the pending load-state request (F2).
 * @return true once if F2 was pressed since the last call.
 */
auto FrontendQt::pop_load_request() -> bool
{
    return _load_requested.exchange(false);
}

/**
 * @brief Consume and return the pending ROM load request raised by @ref load_rom.
 *
 * @param filename Output: path of the ROM to load, only set when true is returned.
 * @return true once if a ROM was selected since the last call.
 */
auto FrontendQt::pop_rom_request(std::string &filename) -> bool
{
    if (!_rom_requested.exchange(false))
        return false;
    filename = _rom_path;
    return true;
}

/**
 * @brief Sleeps until the next frame deadline, then advances it by one frame period.
 */
auto FrontendQt::delay() -> void
{
    qint64 now = _elapsed_timer.nsecsElapsed();
    qint64 diff = _next_frame - now;

    if (now < _next_frame)
        QThread::sleep(std::chrono::nanoseconds(diff));
    else
        _next_frame = now;
    _next_frame += _increment_frame;
}

/**
 * @brief Applies the high-pass filter if active, then sends the buffer to the Qt audio sink.
 * @param buffer View of the audio samples produced by the APU.
 */
auto FrontendQt::play_audio(std::span<const int16_t> buffer) -> void
{
    qint64 total = static_cast<qint64>(buffer.size()) * sizeof(int16_t);
    qint64 done = 0;
    qint64 writed = 0;

    if (buffer.empty() || _audio_device == nullptr)
        return;

    if (_audio_buffer.size() != buffer.size())
        _audio_buffer.resize(buffer.size());
    std::copy(buffer.begin(), buffer.end(), _audio_buffer.begin());

    if (_config._audio_filter)
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
 * @brief Copies the de-padded LCD frame into the shared framebuffer and pushes it to the QML item.
 * @param buffer RGBA8888 frame produced by Emulator::get_graphic_buffer().
 */
auto FrontendQt::render(std::span<const uint8_t> buffer) -> void
{
    QMutexLocker locker(&_framebuffer.get_mutex());

    if (buffer.size() == graphic_buffer_size)
    {
        _framebuffer.get_image() = QImage(buffer.data(), screen_width, screen_height, QImage::Format_RGBA8888).copy();
        emit frameChanged();
    }
}

/**
 * @brief Returns the audio sample rate used by the audio sink.
 * @return Sample rate in Hz.
 */
auto FrontendQt::get_samplerate() -> uint32_t
{
    return _samplerate;
}

/**
 * @brief Reports an error message through the Qt logging system.
 * @param message Message to report.
 */
auto FrontendQt::log(std::string_view message) -> void
{
    qWarning().noquote() << QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size()));
}

/**
 * @brief Requests the loading of a ROM file selected from the QML interface.
 *
 * Only stores the selected path and raises the request flag; the emulation thread
 * performs the load when it consumes the request through @ref pop_rom_request.
 *
 * @param filename URL of the selected ROM, as produced by the QML FileDialog (file:// scheme).
 */
auto FrontendQt::load_rom(const QString &filename) -> void
{
    _rom_path = QUrl(filename).toLocalFile().toStdString();
    _rom_requested = true;
}

/**
 * @brief Changes the screen scale factor from the QML interface.
 *
 * The window size follows through the screenWidth / screenHeight properties, which
 * emit @ref resolutionChanged when the scale changes.
 *
 * @param scale New integer scale factor (must be positive).
 */
auto FrontendQt::set_scale(int scale) -> void
{
    if (scale <= 0 || scale == _scale)
        return;
    _scale = scale;
    emit resolutionChanged();
}

/**
 * @brief Raises a save-state request from the QML interface, as pressing F1 does.
 *
 * Only sets the flag; the emulation thread performs the save when it consumes the
 * request through @ref pop_save_request.
 */
auto FrontendQt::save_state() -> void
{
    _save_requested = true;
}

/**
 * @brief Raises a load-state request from the QML interface, as pressing F2 does.
 *
 * Only sets the flag; the emulation thread performs the load when it consumes the
 * request through @ref pop_load_request.
 */
auto FrontendQt::load_state() -> void
{
    _load_requested = true;
}

/**
 * @brief Registers the QML types, installs the event filter and loads the QML interface.
 *
 * The application metadata and the translator are set up before the QML is loaded:
 * the interface reads Qt.application.name / Qt.application.version while it is being
 * instantiated, and qsTr() resolves against the installed translator at load time.
 */
auto FrontendQt::init_qt() -> void
{
    _app.installEventFilter(&_controllers);
    _app.setApplicationName(QString(app_name.data()));
    _app.setApplicationVersion(QString(app_version.data()));
    set_language();
    _engine.rootContext()->setContextProperty("FrontendQt", this);
    _engine.rootContext()->setContextProperty("ControllersQt", &_controllers);
    _engine.loadFromModule("Gbemu", "Main");
    if (_engine.rootObjects().isEmpty())
        throw std::runtime_error("Failed to load QML interface");
}

/**
 * @brief Switches the interface language, falling back to the system locale.
 *
 * The requested code is used when its embedded translation exists; otherwise the
 * translation matching the system locale is loaded. The QML engine then
 * retranslates every qsTr() binding so the interface updates immediately.
 *
 * @param code Language code (e.g. "fr", "en"). Empty selects the system locale.
 */
auto FrontendQt::set_language(const QString &code) -> void
{
    auto translator = std::make_unique<QTranslator>();
    const QString prefix = QString(":/i18n/%1_").arg(app_name.data());

    if (!code.isEmpty() && translator->load(prefix + code + ".qm"))
        _language = code;
    else if (translator->load(QLocale(), app_name.data(), "_", ":/i18n"))
        _language = QLocale().name().section('_', 0, 0);
    else
    {
        qWarning() << "No translation available for" << code;
        return;
    }

    if (_translator)
        _app.removeTranslator(_translator.get());
    _translator = std::move(translator);
    _app.installTranslator(_translator.get());
    emit languageChanged();
    _engine.retranslate();
}

/**
 * @brief Opens the Qt audio sink and starts playback.
 *
 * The sample rate is taken from the preferred format of the default audio device
 * when one is available.
 */
auto FrontendQt::init_audio() -> void
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
