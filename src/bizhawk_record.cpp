#include "bizhawk_record.h"
#include "controllers.h"
#include <array>
#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <zip.h>

/**
 * @brief Parse a BizHawk BK2 record file and populate the input vectors.
 *
 * The BK2 file is a ZIP archive. This method extracts header.txt to read
 * the ROM SHA1 and Input Log.txt to load frame inputs.
 *
 * @param filename Path to the BK2 file.
 * @throws std::runtime_error if the archive cannot be opened or a required entry is missing.
 */
auto BizHawkRecord::parse_file(std::string_view filename) -> void
{
    int err = 0;
    zip_t *raw = zip_open(filename.data(), ZIP_RDONLY, &err);
    ZipHandle archive{raw, zip_close};

    if (!raw)
    {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        std::string msg = std::format("Failed to open record file: {} — {}", filename, zip_error_strerror(&error));
        zip_error_fini(&error);
        throw std::runtime_error(msg);
    }

    read_zip_entry(archive, header_filename);
    parse_header();

    read_zip_entry(archive, input_log_filename);
    parse_input_log();
}

/**
 * @brief Read the full content of a named entry from an open ZIP archive into _zip_buffer.
 *
 * @param archive Open ZIP archive handle.
 * @param name    Name of the entry to read.
 * @throws std::runtime_error if the entry is not found or cannot be read.
 */
auto BizHawkRecord::read_zip_entry(ZipHandle &archive, std::string_view name) -> void
{
    zip_file_t *file = zip_fopen(archive.get(), name.data(), 0);
    if (!file)
        throw std::runtime_error(std::format("Entry not found in archive: {}", name));

    _zip_buffer.clear();
    std::array<uint8_t, zip_block_size> buf{};
    zip_int64_t n;
    while ((n = zip_fread(file, buf.data(), buf.size())) > 0)
        _zip_buffer.insert(_zip_buffer.end(), buf.begin(), buf.begin() + n);
    zip_fclose(file);
}

/**
 * @brief Parse the header.txt content from _zip_buffer and extract the ROM SHA1.
 */
auto BizHawkRecord::parse_header() -> void
{
    std::istringstream stream{std::string(_zip_buffer.begin(), _zip_buffer.end())};
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.rfind("SHA1 ", 0) == 0)
        {
            _sha1 = line.substr(5);
            while (!_sha1.empty() && (_sha1.back() == '\r' || _sha1.back() == ' '))
                _sha1.pop_back();
            break;
        }
    }
}

/**
 * @brief Parse the Input Log.txt content from _zip_buffer and populate the input vectors.
 *
 * First pass: reads the LogKey line and builds two lookup tables (pad and button),
 * each of 4 entries indexed by bit position, storing {field_index, char_pos} pairs.
 * A '#' prefix on a token marks the start of a new '|'-separated field in the frame.
 * Player prefixes (e.g. "P1 ") are stripped by taking the last word of each token.
 * Second pass: for each frame line, splits on '|' into fields, then iterates both
 * tables to set the active bits in _dpads_records and _buttons_records.
 */
auto BizHawkRecord::parse_input_log() -> void
{
    std::istringstream stream{std::string(_zip_buffer.begin(), _zip_buffer.end())};
    std::string line;

    // {field_index, char_pos_in_field}: index = bit position (0=RIGHT/A, 1=LEFT/B, 2=UP/SELECT, 3=DOWN/START)
    using ButtonPos = std::pair<int, int>;
    std::array<ButtonPos, 4> pad_table{};
    std::array<ButtonPos, 4> btn_table{};
    pad_table.fill({-1, -1});
    btn_table.fill({-1, -1});

    // --- decode LogKey ---
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind("LogKey:", 0) != 0)
            continue;

        std::string key = line.substr(7);
        if (!key.empty() && key[0] == '#')
            key = key.substr(1);

        int field = 0;
        int col_in_field = 0;
        std::string token;

        for (char c : key)
        {
            if (c != '|')
            {
                token += c;
                continue;
            }
            // '#' prefix means this token starts a new '|'-separated field in the frame
            if (!token.empty() && token[0] == '#')
            {
                field++;
                col_in_field = 0;
                token = token.substr(1);
            }
            // Strip player prefix (e.g. "P1 "): use the last word after the final space
            std::size_t space = token.rfind(' ');
            std::string_view btn =
                (space != std::string::npos) ? std::string_view(token).substr(space + 1) : std::string_view(token);

            if (btn == "Up")
                pad_table[2] = {field, col_in_field}; // bit 2 = UP (0x04)
            else if (btn == "Down")
                pad_table[3] = {field, col_in_field}; // bit 3 = DOWN (0x08)
            else if (btn == "Left")
                pad_table[1] = {field, col_in_field}; // bit 1 = LEFT (0x02)
            else if (btn == "Right")
                pad_table[0] = {field, col_in_field}; // bit 0 = RIGHT (0x01)
            else if (btn == "Start")
                btn_table[3] = {field, col_in_field}; // bit 3 = START (0x08)
            else if (btn == "Select")
                btn_table[2] = {field, col_in_field}; // bit 2 = SELECT (0x04)
            else if (btn == "B")
                btn_table[1] = {field, col_in_field}; // bit 1 = B (0x02)
            else if (btn == "A")
                btn_table[0] = {field, col_in_field}; // bit 0 = A (0x01)
            token.clear();
            col_in_field++;
        }
        break;
    }

    // --- decode frame lines ---
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] != '|')
            continue;

        // Split the frame on '|' into fields (skip the leading '|')
        std::vector<std::string> fields;
        std::string field_tok;
        for (char c : line.substr(1))
        {
            if (c == '|')
            {
                fields.push_back(field_tok);
                field_tok.clear();
            }
            else
                field_tok += c;
        }
        if (!field_tok.empty())
            fields.push_back(field_tok);

        int dpads = 0xF;
        int buttons = 0xF;

        auto is_pressed = [&](ButtonPos pos) -> bool {
            auto [f, col] = pos;
            return f >= 0 && f < static_cast<int>(fields.size()) && col >= 0 &&
                   col < static_cast<int>(fields[f].size()) && fields[f][col] != '.';
        };

        for (int bit = 0; bit < 4; bit++)
        {
            if (is_pressed(pad_table[bit]))
                dpads &= ~(1 << bit);
            if (is_pressed(btn_table[bit]))
                buttons &= ~(1 << bit);
        }
        _dpads_records.push_back(static_cast<uint8_t>(dpads));
        _buttons_records.push_back(static_cast<uint8_t>(buttons));
    }
}
