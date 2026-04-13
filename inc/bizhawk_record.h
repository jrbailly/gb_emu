#ifndef _BIZHAWK_RECORD_H_
#define _BIZHAWK_RECORD_H_

#include "irecord.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct zip;
using ZipHandle = std::unique_ptr<zip, int (*)(zip *)>;

static constexpr std::size_t zip_block_size = 4096;
static constexpr std::string_view header_filename = "Header.txt";
static constexpr std::string_view input_log_filename = "Input Log.txt";

class BizHawkRecord : public IRecord
{
  public:
    auto parse_file(std::string_view filename) -> void override;
    inline auto get_sha1() const -> std::string_view
    {
        return _sha1;
    }

  private:
    auto read_zip_entry(ZipHandle &archive, std::string_view name) -> void;
    auto parse_header() -> void;
    auto parse_input_log() -> void;

  private:
    std::string _sha1;
    std::vector<uint8_t> _zip_buffer;
};

#endif
