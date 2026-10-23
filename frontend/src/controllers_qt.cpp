#include "controllers_qt.h"
#include "controllers.h"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QSet>
#include <QVariantMap>
#include <algorithm>

/**
 * @brief Constructs the controller with the settings shared by the Qt frontend.
 *
 * @param settings Settings instance owned by the frontend.
 */
ControllersQt::ControllersQt(QSettings &settings) : _settings(settings)
{
}

/**
 * @brief Initializes the keyboard input bindings.
 */
auto ControllersQt::init_controllers() -> void
{
    _dpads_binding_keys[Qt::Key_Z] = {QT_TRANSLATE_NOOP("ControllersQt", "Up"), Controllers::UP,
                                      QKeySequence(Qt::Key_Z).toString(QKeySequence::NativeText), 0};
    _dpads_binding_keys[Qt::Key_S] = {QT_TRANSLATE_NOOP("ControllersQt", "Down"), Controllers::DOWN,
                                      QKeySequence(Qt::Key_S).toString(QKeySequence::NativeText), 1};
    _dpads_binding_keys[Qt::Key_Q] = {QT_TRANSLATE_NOOP("ControllersQt", "Left"), Controllers::LEFT,
                                      QKeySequence(Qt::Key_Q).toString(QKeySequence::NativeText), 2};
    _dpads_binding_keys[Qt::Key_D] = {QT_TRANSLATE_NOOP("ControllersQt", "Right"), Controllers::RIGHT,
                                      QKeySequence(Qt::Key_D).toString(QKeySequence::NativeText), 3};
    _buttons_binding_keys[Qt::Key_Backspace] = {QT_TRANSLATE_NOOP("ControllersQt", "Select"), Controllers::SELECT,
                                                QKeySequence(Qt::Key_Backspace).toString(QKeySequence::NativeText), 4};
    _buttons_binding_keys[Qt::Key_Return] = {QT_TRANSLATE_NOOP("ControllersQt", "Start"), Controllers::START,
                                             QKeySequence(Qt::Key_Return).toString(QKeySequence::NativeText), 5};
    _buttons_binding_keys[Qt::Key_L] = {QT_TRANSLATE_NOOP("ControllersQt", "A"), Controllers::A,
                                        QKeySequence(Qt::Key_L).toString(QKeySequence::NativeText), 6};
    _buttons_binding_keys[Qt::Key_K] = {QT_TRANSLATE_NOOP("ControllersQt", "B"), Controllers::B,
                                        QKeySequence(Qt::Key_K).toString(QKeySequence::NativeText), 7};

    load_settings();
    emit keyBindingsChanged();
}

/**
 * @brief Returns all keyboard bindings sorted by their display order.
 *
 * Each element holds the controller name, the Qt key code (also used as the
 * rebinding identifier) and the key display text.
 *
 * @return List of the eight bindings, d-pad first.
 */
auto ControllersQt::keyBindings() const -> QVariantList
{
    QVariantList bindings;

    for (const auto &[key_code, binding] : _dpads_binding_keys.asKeyValueRange())
        bindings.append(QVariantMap{
            {"controller", QCoreApplication::translate("ControllersQt", binding.controller_name.toUtf8().constData())},
            {"keyCode", key_code},
            {"keyText", QKeySequence(key_code).toString(QKeySequence::NativeText)},
            {"order", binding.order}});

    for (const auto &[key_code, binding] : _buttons_binding_keys.asKeyValueRange())
        bindings.append(QVariantMap{
            {"controller", QCoreApplication::translate("ControllersQt", binding.controller_name.toUtf8().constData())},
            {"keyCode", key_code},
            {"keyText", QKeySequence(key_code).toString(QKeySequence::NativeText)},
            {"order", binding.order}});

    std::sort(bindings.begin(), bindings.end(), [](const QVariant &left, const QVariant &right) {
        return left.toMap().value("order").toInt() < right.toMap().value("order").toInt();
    });

    return bindings;
}

/**
 * @brief Restores one binding table from the settings, keeping the defaults when absent.
 *
 * @param bindings Binding table to update, indexed by Qt key code.
 */
auto ControllersQt::load_bindings(QMap<int, KeyBinding> &bindings) -> void
{
    QMap<int, KeyBinding> loaded;

    for (const auto &[default_code, binding] : bindings.asKeyValueRange())
    {
        const QString setting_key = QStringLiteral("bind_%1").arg(binding.order);
        int key_code = default_code;

        if (_settings.contains(setting_key))
        {
            bool ok = false;
            const int stored_code = _settings.value(setting_key).toInt(&ok);

            if (ok && stored_code > 0)
                key_code = stored_code;
            else
                qWarning() << "ControllersQt: invalid key code for" << setting_key << ", using default";
        }

        loaded.insert(key_code, {binding.controller_name, binding.mask,
                                 QKeySequence(key_code).toString(QKeySequence::NativeText), binding.order});
    }
    bindings = loaded;
}

/**
 * @brief Restores the key bindings from the settings, keeping the defaults when absent.
 */
auto ControllersQt::load_settings() -> void
{
    _settings.beginGroup(QStringLiteral("keyboard"));
    load_bindings(_dpads_binding_keys);
    load_bindings(_buttons_binding_keys);
    _settings.endGroup();
}

/**
 * @brief Writes the current key bindings, one entry per binding order.
 */
auto ControllersQt::save_settings() -> void
{
    _settings.beginGroup(QStringLiteral("keyboard"));
    _settings.remove(QString());

    for (const auto &[key_code, binding] : _dpads_binding_keys.asKeyValueRange())
        _settings.setValue(QStringLiteral("bind_%1").arg(binding.order), key_code);

    for (const auto &[key_code, binding] : _buttons_binding_keys.asKeyValueRange())
        _settings.setValue(QStringLiteral("bind_%1").arg(binding.order), key_code);

    _settings.endGroup();
    _settings.sync();
}

/**
 * @brief Moves the binding of the key being rebound to a new key.
 *
 * @param key_code Qt key code of the key just pressed.
 */
auto ControllersQt::rebind_key(int key_code) -> void
{
    if (_dpads_binding_keys.contains(_rebinding_key))
    {
        KeyBinding binding = _dpads_binding_keys.take(_rebinding_key);
        _dpads_binding_keys.insert(key_code,
                                   {binding.controller_name, binding.mask,
                                    QKeySequence(key_code).toString(QKeySequence::NativeText), binding.order});
    }
    else if (_buttons_binding_keys.contains(_rebinding_key))
    {
        KeyBinding binding = _buttons_binding_keys.take(_rebinding_key);
        _buttons_binding_keys.insert(key_code,
                                     {binding.controller_name, binding.mask,
                                      QKeySequence(key_code).toString(QKeySequence::NativeText), binding.order});
    }

    _rebinding_key = -1;
    emit rebindingChanged();
    emit keyBindingsChanged();
    save_settings();
}

/**
 * @brief Sets the key whose binding must be replaced by the next key pressed.
 *
 * @param key_code Qt key code of the binding to rebind, or a negative value to cancel.
 */
auto ControllersQt::set_rebinding_key(int key_code) -> void
{
    _rebinding_key = key_code;
    emit rebindingChanged();
}

/**
 * @brief Filters Qt application events to update the input state and request flags.
 *
 * Key press and release events update the internal d-pad and button bitmasks.
 * F1 and F2 raise the save or load requests through @ref save_state and
 * @ref load_state, which are consumed by @ref pop_save_request and @ref pop_load_request.
 *
 * @param object Watched object (unused).
 * @param event  Event being filtered.
 * @return true if the event should be filtered out, false otherwise.
 */
auto ControllersQt::eventFilter(QObject *object, QEvent *event) -> bool
{
    QKeyEvent *key = nullptr;

    switch (event->type())
    {
    case QEvent::KeyPress:
        key = static_cast<QKeyEvent *>(event);
        if (_rebinding_key >= 0)
        {
            rebind_key(key->key());
            return true;
        }
        if (!key->isAutoRepeat())
        {
            if (_dpads_binding_keys.count(key->key()))
                _dpads &= ~_dpads_binding_keys[key->key()].mask;
            if (_buttons_binding_keys.count(key->key()))
                _buttons &= ~_buttons_binding_keys[key->key()].mask;
        }
        break;
    case QEvent::KeyRelease:
        key = static_cast<QKeyEvent *>(event);
        if (!key->isAutoRepeat())
        {
            if (_dpads_binding_keys.count(key->key()))
                _dpads |= _dpads_binding_keys[key->key()].mask;
            if (_buttons_binding_keys.count(key->key()))
                _buttons |= _buttons_binding_keys[key->key()].mask;
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(object, event);
}
