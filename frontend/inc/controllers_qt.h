#ifndef _CONTROLLERS_QT_H_
#define _CONTROLLERS_QT_H_

#include <QEvent>
#include <QMap>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariantList>
#include <atomic>
#include <cstdint>

struct KeyBinding
{
    QString controller_name;
    uint8_t mask;
    QString key_name;
    int order;
};

class ControllersQt : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList keyBindings READ keyBindings NOTIFY keyBindingsChanged)
    Q_PROPERTY(int rebindingKey READ rebinding_key WRITE set_rebinding_key NOTIFY rebindingChanged)

  public:
    explicit ControllersQt(QSettings &settings);
    virtual ~ControllersQt() override = default;

    auto init_controllers() -> void;
    auto eventFilter(QObject *watched, QEvent *event) -> bool override;
    auto keyBindings() const -> QVariantList;
    auto load_settings() -> void;
    auto save_settings() -> void;
    auto set_rebinding_key(int key_code) -> void;
    auto rebinding_key() const -> int
    {
        return _rebinding_key;
    }
    auto get_dpads() const -> uint8_t
    {
        return _dpads;
    }
    auto get_buttons() const -> uint8_t
    {
        return _buttons;
    }

  signals:
    auto keyBindingsChanged() -> void;
    auto rebindingChanged() -> void;
    auto bindingPersisted() -> void;

  private:
    auto load_bindings(QMap<int, KeyBinding> &bindings) -> void;
    auto rebind_key(int key_code) -> void;

  private:
    std::atomic<uint8_t> _dpads{0xFF};
    std::atomic<uint8_t> _buttons{0xFF};
    QMap<int, KeyBinding> _dpads_binding_keys;
    QMap<int, KeyBinding> _buttons_binding_keys;
    QSettings &_settings;
    int _rebinding_key{-1};
};

#endif
