#pragma once
#include <array>
#include <cstdint>
#include <span>

static constexpr std::size_t npole = 1;

class HighpassFilter
{
  public:
    HighpassFilter();
    auto filter(std::span<int16_t> buffer, std::size_t channel_offset, std::size_t step) -> void;

  private:
    float _gain;
    std::array<float, npole + 1> _acoeff;
    std::array<float, npole + 1> _bcoeff;
    std::array<float, npole + 1> _hfilter_x;
    std::array<float, npole + 1> _hfilter_y;
};