#ifndef _TIMER_H_
#define _TIMER_H_

#include "ram.h"

const int cycles_div = 256;
class Timer
{
  public:
    enum Register
    {
        DIV = 0xFF04,
        TIMA = 0xFF05,
        TMA = 0xFF06,
        TAC = 0xFF07
    };
    Timer();
    void init(MBC1 &ram);
    void step(MBC1 &ram, uint32_t cycles_count);
    void reset();

  private:
    int mNextCycleDiv;
    int mNextCycleTima;
    int mCycleTima;
    int mClocksCycles[4];
};
#endif