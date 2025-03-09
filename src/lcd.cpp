#include "lcd.h"
#include "ram.h"
#include <cstdio>

LCD::LCD(MBC1 &ram) : mRAM(ram)
{
    mNext_line_cycle = 456;
    mWindow = SDL_CreateWindow("", 160, 144, 0);
    mRAM.write(Register::LCDC, 0x82);
}

void LCD::step(uint32_t cycles_count)
{
    if (cycles_count > mNext_line_cycle)
    {
        uint8_t ly = mRAM[Register::LY] + 1;
        mRAM.write(Register::LY,  ly);
        mNext_line_cycle += 456;

        // vblank
        if (ly == 144)
        {
            mRAM.write(CPU::Register::IF, 0x1);
            mRAM.write(Register::STAT, 0x1);
        }
    }
}

void LCD::render()
{
}

void LCD::reset ()
{
    mNext_line_cycle = 456;
    mRAM.write(Register::LY, 0);
    mRAM.write(Register::STAT, 0x0);
}
