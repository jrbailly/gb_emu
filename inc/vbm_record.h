#ifndef _VBM_RECORD_H_
#define _VBM_RECORD_H_

#include "controllers.h"
#include "irecord.h"

/**
 * @brief IRecord implementation for the VBM text-based record format.
 *
 * Each input line starts with '|' followed by one character per button:
 * position 1-4 = Up/Down/Left/Right, position 5-8 = Start/Select/B/A.
 * A '.' character means the button is released; any other character means pressed.
 */
class VbmRecord : public IRecord
{
  public:
    /**
     * @brief Parse a VBM record file and populate the input vectors.
     * @param filename Path to the VBM record file.
     * @throws std::runtime_error if the file cannot be opened.
     */
    auto parse_file(const std::string &filename) -> void override;
};

#endif
