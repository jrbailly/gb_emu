#include "timer.h"
#include "cpu.h"

Timer::Timer(MBC1 &RAM) : mRAM(RAM), mNextCycleDiv(0), mNextCycleTima(0), mCycleTima(0)
{
    mClocksCycles[0] = 1024;
    mClocksCycles[1] = 16;
    mClocksCycles[2] = 64;
    mClocksCycles[3] = 256;
}

void Timer::step(int cycles, uint16_t last_addr)
{
    if (mNextCycleDiv <= 0)
    {
        uint8_t div = mRAM[Register::DIV] + 1;

        mRAM.write(Register::DIV, div);
        mNextCycleDiv = cycles_div;
    }
    if (mNextCycleTima <= 0 && mCycleTima > 0)
    {
        uint8_t tima = mRAM[Register::TIMA];
        uint8_t tma = mRAM[Register::TMA];

        if (tima == 0xFF)
        {
            tima = tma;
            mRAM.write(CPU::Register::IF, 0x4);
        }
        else
            tima++;
        mRAM.write(Register::TIMA, tima);
        mNextCycleTima = mCycleTima;
    }
    if (last_addr == 0xFF07)
    {
        uint8_t clock = mRAM[Register::TAC] & 0x3;

        if (mRAM[Register::TAC] & 0x4)
        {
            mCycleTima = mClocksCycles[clock];
            mNextCycleTima = cycles + mCycleTima;
        }
        else
            mCycleTima = 0;
    }
    mNextCycleDiv -= cycles;
    mNextCycleTima -= cycles;
}

void Timer::reset()
{
    mNextCycleDiv = 0;
    mNextCycleTima = 0;
}