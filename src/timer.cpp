#include "timer.h"
#include "cpu.h"

Timer::Timer() : mNextCycleDiv(0), mNextCycleTima(0), mCycleTima(0)
{
    mClocksCycles[0] = 1024;
    mClocksCycles[1] = 16;
    mClocksCycles[2] = 64;
    mClocksCycles[3] = 256;
}

void Timer::init(MBC1 &ram)
{
    ram.RegisterCallback(Register::TAC, [this](MBC1 &ram, uint16_t addr, uint8_t val) {
        uint8_t clock = val & 0x3;

        if (val & 0x4)
        {
            mCycleTima = mClocksCycles[clock];
            mNextCycleTima = mCycleTima;
        }
        else
            mCycleTima = 0;
    });
}

void Timer::step(MBC1 &ram, uint32_t cycles_count)
{
    if (mNextCycleDiv <= 0)
    {
        uint8_t div = ram[Register::DIV] + 1;

        ram.write(Register::DIV, div);
        mNextCycleDiv = cycles_div + cycles_count;
    }
    if (mNextCycleTima <= 0 && mCycleTima > 0)
    {
        uint8_t tima = ram[Register::TIMA];
        uint8_t tma = ram[Register::TMA];

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