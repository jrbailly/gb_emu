#include "gb_framebuffer_item.h"
#include <QQuickWindow>
#include <QSGSimpleTextureNode>

/**
 * @brief Constructs the framebuffer item.
 *
 * The pixels themselves are not owned by the item: it renders whatever the
 * @ref Framebuffer it is pointed at currently holds.
 *
 * @param parent Parent item.
 */
GBFramebufferItem::GBFramebufferItem(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

/**
 * @brief Points the item at the shared framebuffer and schedules a repaint.
 *
 * Only stores the reference; the pixels are read from the framebuffer at paint
 * time, so nothing is copied here.
 *
 * @param framebuffer Framebuffer published by the frontend, or nullptr to clear.
 */
auto GBFramebufferItem::setFrame(Framebuffer *framebuffer) -> void
{
    _framebuffer = framebuffer;
    _frame_dirty = true;
    update();
}

/**
 * @brief Builds or updates the scene graph node used to display the framebuffer.
 *
 * The whole read — texture upload and geometry — is performed under the
 * framebuffer's mutex, on its image directly, so the emulation thread never
 * mutates the pixels mid-read. The texture is only re-uploaded when a new frame
 * was signalled, then the source rectangle is restricted to the Game Boy screen
 * area inside the padded image.
 *
 * @param oldNode Previously created node, or nullptr on the first call.
 * @param data    Qt scene graph update data (unused).
 * @return The scene graph node to render, or nullptr if no frame is available.
 */
auto GBFramebufferItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData * /*data*/) -> QSGNode *
{
    auto *node = static_cast<QSGSimpleTextureNode *>(oldNode);

    if (_framebuffer == nullptr)
        return nullptr;
    QMutexLocker locker(&((*_framebuffer).get_mutex()));
    auto& image = (*_framebuffer).get_image();

    if (image.isNull ())
        return nullptr;
    if (!node)
    {
        node = new QSGSimpleTextureNode();
        node->setFiltering(QSGTexture::Nearest);
        node->setOwnsTexture(true);
    }
    if (_frame_dirty)
    {
        auto texture = window()->createTextureFromImage(image);
        if (texture)
        {
            node->setTexture(texture);
            _frame_dirty = false;
        }
    }
    node->setSourceRect(QRectF(0, 0, image.width(), image.height()));
    node->setRect(boundingRect());
    return node;
}
