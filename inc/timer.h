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
    Timer(MBC1 &RAM);
    void step(int cycles_count, uint16_t last_addr);
    void reset();

  private:
    MBC1 &mRAM;
    int mNextCycleDiv;
    int mNextCycleTima;
    int mCycleTima;
    int mClocksCycles[4];
};
#endif