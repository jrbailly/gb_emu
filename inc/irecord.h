#ifndef _IRECORD_H_
#define _IRECORD_H_

#include <string>
#include <vector>

/**
 * @brief Abstract interface for input record playback.
 *
 * Subclasses implement @ref parse_file to load a specific record format.
 * The base class provides frame-by-frame input retrieval via @ref get_input
 * and index reset via @ref reset.
 */
class IRecord
{
  public:
    virtual ~IRecord() = default;

    /**
     * @brief Parse a record file and populate the input vectors.
     * @param filename Path to the record file.
     */
    virtual auto parse_file(const std::string &filename) -> void = 0;

    /**
     * @brief Return the inputs for the current frame and advance to the next one.
     *
     * The index wraps around when it reaches the end of the record.
     *
     * @param pad    Output: d-pad state for the current frame.
     * @param button Output: button state for the current frame.
     * @return true if inputs were available, false if the record is empty.
     */
    auto get_input(int &pad, int &button) -> bool;

    /**
     * @brief Reset the playback position to the beginning of the record.
     */
    auto reset() -> void;

  protected:
    std::vector<int> _dpads_records;   ///< D-pad state for each recorded frame.
    std::vector<int> _buttons_records; ///< Button state for each recorded frame.

  private:
    int _record_index = 0; ///< Current playback position.
};

#endif
