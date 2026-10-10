#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_

#include <QImage>
#include <QMutex>

/**
 * @brief Shared frame buffer between the emulation thread and the Qt scene graph.
 *
 * Holds the last rendered frame as a QImage together with the QMutex that guards
 * concurrent access from the emulation thread (producer) and the UI thread
 * (consumer). Only the raw storage and its direct accessors live here.
 */
class Framebuffer
{
  public:
    Framebuffer() = default;
    ~Framebuffer() = default;

    /**
     * @brief Access the frame image directly.
     *
     * @return Reference to the frame image.
     */
    auto get_image() -> QImage &
    {
        return _image;
    }

    /**
     * @brief Access the frame image directly, read-only.
     *
     * @return Const reference to the frame image.
     */
    auto get_image() const -> const QImage &
    {
        return _image;
    }

    /**
     * @brief Access the mutex protecting the frame image directly.
     *
     * @return Reference to the mutex.
     */
    auto get_mutex() -> QMutex &
    {
        return _mutex;
    }

  private:
    QImage _image;
    QMutex _mutex;
};

#endif
