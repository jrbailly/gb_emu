#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "emulator.h"
#include "ifrontend.h"
#include "irecord.h"

static constexpr std::string_view app_version = "0.1.0";

class Application
{
  public:
    Application();
    auto init(const Config &configuration) -> void;
    auto main_loop() -> void;

  private:
    std::unique_ptr<IFrontend> _frontend;
    std::unique_ptr<Emulator> _emulator;
    std::unique_ptr<IRecord> _record;
    bool _quit = false;
};
#endif