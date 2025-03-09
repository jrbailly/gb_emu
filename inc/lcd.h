#ifndef _LCD_H_
#define _LCD_H_

#include "ram.h"
#include "cpu.h"
#include<SDL3/SDL.h>

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
    void reset ();
  private:
    MBC1 &mRAM;
    uint32_t mNext_line_cycle;
    SDL_Window* mWindow;
};

#endif