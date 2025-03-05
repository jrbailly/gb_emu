#include "lcd.h"
#include "ram.h"

LCD::LCD(MBC1 &ram) : mRAM(ram)
{
    mNext_line_cycle = 456;
}

void LCD::step(uint32_t cycles_count)
{
    if (cycles_count > mNext_line_cycle)
    {
        mRAM.write(Register::LY, mRAM[Register::LY] + 1);
        mNext_line_cycle += 456;
    }
}

void LCD::render()
{
    mRAM.write(Register::LY, 0);
    mNext_line_cycle = 456;
}