#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "emulator.h"
#include "ifrontend.h"

class Application
{
  public:
    Application();
    auto MainLoop(const Config &Configuration) -> void;

  private:
    std::unique_ptr<IFrontend> mFrontend;
    std::unique_ptr<Emulator> mEmulator;
};
#endif