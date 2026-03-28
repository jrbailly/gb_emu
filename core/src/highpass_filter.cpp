#include "highpass_filter.h"

/**
 * @brief Constructor for the HighpassFilter class.
 */
HighpassFilter::HighpassFilter()
{
    _acoeff = {-0.9929014613741289, 1};
    _bcoeff = {-1, 1};
    _gain = 1.003561911295743;
    _hfilter_x = {0, 0};
    _hfilter_y = {0, 0};
}

/**
 * @brief Applies a Butterworth 3 order high-pass filter to the audio signal with a cutoff frequency of 50hz to remove
 * DC.
 * @param v The input audio sample value.
 * @param channel The audio channel (0 or 1).
 * @return The filtered audio sample value.
 */
auto HighpassFilter::filter(std::span<int16_t> buffer, int channel_offset, int step) -> void
{
    int i;
    float out = 0;

    for (int j = channel_offset; j < (int)buffer.size(); j += step)
    {
        float value = buffer[j];
        out = 0;
        for (i = 0; i < npole; i++)
            _hfilter_x[i] = _hfilter_x[i + 1];
        _hfilter_x[npole] = value / _gain;
        for (i = 0; i < npole; i++)
            _hfilter_y[i] = _hfilter_y[i + 1];
        for (i = 0; i <= npole; i++)
            out += _hfilter_x[i] * _bcoeff[i];
        for (i = 0; i < npole; i++)
            out -= _hfilter_y[i] * _acoeff[i];
        _hfilter_y[npole] = out;
        buffer[j] = static_cast<int16_t>(out);
    }
}