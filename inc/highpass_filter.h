#pragma once
#include <array>
#include <cstdint>

static constexpr int NPOLE = 1;

class HighpassFilter
{
  public:
    HighpassFilter();
    auto filter(int16_t *buffer, int size, int step) -> void;

  private:
    float _gain;
    std::array<float, NPOLE + 1> _acoeff;
    std::array<float, NPOLE + 1> _bcoeff;
    std::array<float, NPOLE + 1> _hfilter_x;
    std::array<float, NPOLE + 1> _hfilter_y;
};