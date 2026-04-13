#include "vbm_record.h"
#include <format>
#include <fstream>
#include <string>

/**
 * @brief Parse a VBM record file and populate the input vectors.
 *
 * Lines that do not start with '|' are ignored (comments, header, etc.).
 * For each valid line, each character position is checked against '.':
 * a non-dot character clears the corresponding button bit, meaning pressed.
 *
 * @param filename Path to the VBM record file.
 * @throws std::runtime_error if the file cannot be opened.
 */
auto VbmRecord::parse_file(std::string_view filename) -> void
{
    constexpr size_t header_size = 0x100;
    uint16_t frame = 0;
    std::ifstream input(filename.data(), std::ios::binary);

    if (!input)
        throw std::runtime_error(std::format("Failed to open record file: ") + filename.data());
    input.seekg(header_size, std::ios::beg);
    while (input.read(reinterpret_cast<char *>(&frame), sizeof(frame)))
    {
        int dpads = 0xF;
        int buttons = 0xF;

        if (frame & 0x40) // UP
            dpads &= ~Controllers::UP;
        if (frame & 0x80) // DOWN
            dpads &= ~Controllers::DOWN;
        if (frame & 0x20) // LEFT
            dpads &= ~Controllers::LEFT;
        if (frame & 0x10) // RIGHT
            dpads &= ~Controllers::RIGHT;
        if (frame & 0x08) // START
            buttons &= ~Controllers::START;
        if (frame & 0x04) // SELECT
            buttons &= ~Controllers::SELECT;
        if (frame & 0x02) // B
            buttons &= ~Controllers::B;
        if (frame & 0x01) // A
            buttons &= ~Controllers::A;
        _dpads_records.push_back(dpads);
        _buttons_records.push_back(buttons);
    }
    std::cout << _dpads_records.size() << std::endl;
}
