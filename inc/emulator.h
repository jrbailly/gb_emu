#ifndef _EMULATOR_H_
#define _EMULATOR_H_

#include "cartridge.h"
#include "config.h"
#include "controllers.h"
#include "cpu.h"
#include "lcd.h"
#include "ram.h"
#include <memory>

class Emulator
{
  public:
    Emulator(const Config &Configuration);
    virtual ~Emulator();
    void init();
    void loop();

  private:
    std::unique_ptr<CPU> mCPU;
    std::unique_ptr<LCD> mLCD;
    std::unique_ptr<Controllers> mControllers;
    MBC1 mRAM;
    const Config mConfig;
};

#endif