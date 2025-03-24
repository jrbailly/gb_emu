#include "timer.h"
#include "cpu.h"

Timer::Timer() : mNextCycleDiv(0), mNextCycleTima(0), mCycleTima(0)
{
    mClocksCycles[0] = 1024;
    mClocksCycles[1] = 16;
    mClocksCycles[2] = 64;
    mClocksCycles[3] = 256;
}

void Timer::init(RamBus &ram)
{
    ram.register_callback(Register::TAC, [this](RamBus &ram, int addr, unsigned char val) {
        int clock = val & 0x3;

        if (val & 0x4)
        {
            mCycleTima = mClocksCycles[clock];
            mNextCycleTima = mCycleTima;
        }
        else
            mCycleTima = 0;
    });
}

void Timer::step(RamBus &ram, int cycles_count)
{
    if (mNextCycleDiv <= 0)
    {
        unsigned char div = ram[Register::DIV] + 1;

        ram.write(Register::DIV, div);
        mNextCycleDiv = cycles_div + cycles_count;
    }
    if (mNextCycleTima <= 0 && mCycleTima > 0)
    {
        unsigned char tima = ram[Register::TIMA];
        unsigned char tma = ram[Register::TMA];

        if (tima == 0xFF)
        {
            tima = tma;
            ram.write(CPU::Register::IF, 0x4);
        }
        else
            tima++;
        ram.write(Register::TIMA, tima);
        mNextCycleTima = mCycleTima;
    }
    mNextCycleDiv -= cycles_count;
    mNextCycleTima -= cycles_count;
}

void Timer::reset()
{
    mNextCycleDiv = 0;
    mNextCycleTima = 0;
}