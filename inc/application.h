#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "emulator.h"
#include "ifrontend.h"
#include "irecord.h"

static constexpr std::string app_version = "0.1.0";

class Application
{
  public:
    Application();
    auto Init(const Config &Configuration) -> void;
    auto MainLoop() -> void;
  private:
    std::unique_ptr<IFrontend> mFrontend;
    std::unique_ptr<Emulator> mEmulator;
    std::unique_ptr<IRecord> mRecord;
    bool mQuit = false;
  };
#endif