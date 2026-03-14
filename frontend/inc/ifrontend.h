#ifndef _IFRONTEND_H_
#define _IFRONTEND_H_

#include <cstdint>
#include <span>

class IFrontend
{
  public:
    virtual ~IFrontend() = default;
    virtual auto poll_inputs() -> bool = 0;
    virtual auto play_audio(std::span<const int16_t> buffer) -> void = 0;
    virtual auto display(const uint8_t *frame_buffer, int width, int height) -> void = 0;
};

#endif
