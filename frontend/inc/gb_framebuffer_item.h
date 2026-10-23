#ifndef _GB_FRAMEBUFFER_ITEM_H_
#define _GB_FRAMEBUFFER_ITEM_H_

#include "framebuffer.h"
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>
#include <atomic>

class GBFramebufferItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Framebuffer *frame READ frame WRITE set_frame)
  public:
    explicit GBFramebufferItem(QQuickItem *parent = nullptr);
    virtual ~GBFramebufferItem() override = default;

    auto frame() -> Framebuffer *
    {
        return _framebuffer;
    }
    auto set_frame(Framebuffer *framebuffer) -> void;

  protected:
    auto updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) -> QSGNode * override;

  private:
    std::atomic<Framebuffer *> _framebuffer{nullptr};
    std::atomic<bool> _frame_dirty{false};
};

#endif
