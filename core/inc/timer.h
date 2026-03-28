#ifndef _TIMER_H_
#define _TIMER_H_

#include "cpu.h"
#include "ram.h"

static constexpr int cycles_per_div_increment = cpu_freq / 16384;
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
    auto init(RamBus &ram) -> void;
    auto step(RamBus &ram, int cycles_count) -> void;

  private:
    int _next_cycle_div;
    int _next_cycle_tima;
    int _cycle_tima;
    int _clocks_cycles[4];
};
#endif