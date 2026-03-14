#include "irecord.h"

/**
 * @brief Return the d-pad and button state for the current frame.
 *
 * Sets @p pad and @p button to the values at the current index, then
 * advances the index (wrapping around to 0 at the end of the record).
 *
 * @param pad    Output: d-pad state for the current frame.
 * @param button Output: button state for the current frame.
 * @return true if inputs were available, false if the record is empty.
 */
auto IRecord::get_input(int &pad, int &button) -> bool
{
    if (_dpads_records.empty())
        return false;
    pad = _dpads_records[_record_index];
    button = _buttons_records[_record_index];
    _record_index = (_record_index + 1) % _dpads_records.size();
    return true;
}

/**
 * @brief Reset the playback position to the first recorded frame.
 */
auto IRecord::reset() -> void
{
    _record_index = 0;
}
