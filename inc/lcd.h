#ifndef _LCD_H_
#define _LCD_H_

#include "ram.h"

class LCD
{
  public:
    enum Register
    {
        LCDC = 0xFF40,
        STAT = 0xFF41,
        SCY = 0xFF42,
        SCX = 0xFF43,
        LY = 0xFF44,
        LYC = 0xFF45
    };
    LCD(MBC1 &ram);
    void step(uint32_t cycles_count);
    void render();

  private:
    MBC1 &mRAM;
    uint32_t mNext_line_cycle;
};

#endif