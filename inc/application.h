#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "emulator.h"
#include "ifrontend.h"
#include "irecord.h"
#include <array>

static constexpr std::string_view app_version = "0.2.0";
static constexpr std::size_t save_state_buffer_size = 64;

class Application
{
  public:
    Application();
    auto init(const Config &configuration) -> void;
    auto main_loop() -> void;

  private:
    auto save_state_json() -> void;
    auto load_state_json() -> void;

    std::unique_ptr<IFrontend> _frontend;
    std::unique_ptr<Emulator> _emulator;
    std::unique_ptr<IRecord> _record;
    std::array<SaveState, save_state_buffer_size> _save_states;
    std::size_t _save_state_index = 0;
    Config _config;
    bool _quit = false;
};
#endif