#include "cpu.h"
#include "iserializable.h"
#include "ram.h"
#include "timer.h"
#include <gtest/gtest.h>

// cycles_per_div_increment = cpu_freq / 16384 = 256
// _clocks_cycles[0] = cpu_freq / 4096   = 1024   (TAC bits 0-1 = 00)
// _clocks_cycles[1] = cpu_freq / 262144 = 16     (TAC bits 0-1 = 01)
// _clocks_cycles[2] = cpu_freq / 65536  = 64     (TAC bits 0-1 = 10)
// _clocks_cycles[3] = cpu_freq / 16384  = 256    (TAC bits 0-1 = 11)
//
// TAC bit 2 enables the timer. Bits 0-1 select the clock source.
// TIMA overflows at 0xFF → TMA is loaded into TIMA and IF bit 2 (TIMER) is set.

class TimerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ram.clear();
        timer.init(ram);
    }

    RamBus ram{true};
    Timer timer{};
};

// ---------------------------------------------------------------------------
// DIV register
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that DIV increments by 1 after exactly cycles_per_div_increment cycles.
 * @details _next_cycle_div is initialized to cycles_per_div_increment (256), so exactly one
 *          step of 256 cycles brings the counter to 0 and triggers the increment.
 */
TEST_F(TimerTest, DIV_IncrementEvery256Cycles)
{
    timer.step(ram, cycles_per_div_increment);
    EXPECT_EQ(ram[Timer::Register::DIV], 1);

    timer.step(ram, cycles_per_div_increment);
    EXPECT_EQ(ram[Timer::Register::DIV], 2);

    timer.step(ram, cycles_per_div_increment);
    EXPECT_EQ(ram[Timer::Register::DIV], 3);
}

/**
 * @brief Verifies that DIV does not increment when fewer than cycles_per_div_increment cycles
 *        have elapsed since the last increment.
 * @details After one full period (256 cycles), _next_cycle_div is back to 256.
 *          Stepping 255 more cycles leaves the counter at 1 — no second increment.
 */
TEST_F(TimerTest, DIV_NoIncrementBeforePeriodElapsed)
{
    timer.step(ram, cycles_per_div_increment); // first increment: DIV = 1
    ASSERT_EQ(ram[Timer::Register::DIV], 1);   // assert: abort if setup is wrong

    timer.step(ram, cycles_per_div_increment - 1);
    EXPECT_EQ(ram[Timer::Register::DIV], 1);   // must not have incremented again
}

/**
 * @brief Verifies that writing any value to DIV clears it to 0.
 * @details The write callback ignores the written value and always stores 0 via write_register.
 */
TEST_F(TimerTest, DIV_WriteAnyClearsToZero)
{
    timer.step(ram, cycles_per_div_increment * 5);
    ASSERT_EQ(ram[Timer::Register::DIV], 5);

    ram.write(Timer::Register::DIV, 0xFF);
    EXPECT_EQ(ram[Timer::Register::DIV], 0);

    ram.write(Timer::Register::DIV, 0x42);
    EXPECT_EQ(ram[Timer::Register::DIV], 0);

    ram.write(Timer::Register::DIV, 0x00);
    EXPECT_EQ(ram[Timer::Register::DIV], 0);
}

/**
 * @brief Verifies that DIV wraps from 0xFF to 0x00 on overflow (8-bit unsigned counter).
 * @details write_register is used to pre-load 0xFF without triggering the clear callback.
 *          One period of cycles causes the 0xFF + 1 = 0x00 unsigned wrap.
 */
TEST_F(TimerTest, DIV_WrapsFrom0xFF_To_0x00)
{
    ram.write_register(Timer::Register::DIV, 0xFF);
    timer.step(ram, cycles_per_div_increment);
    EXPECT_EQ(ram[Timer::Register::DIV], 0x00);
}

// ---------------------------------------------------------------------------
// TIMA — timer disabled
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that TIMA stays at 0 when no TAC write has occurred (timer off by default).
 * @details _cycle_tima is 0 after construction, so the while loop in step() never fires.
 */
TEST_F(TimerTest, TIMA_StaysZeroWhenTimerDisabled)
{
    timer.step(ram, cpu_freq);
    EXPECT_EQ(ram[Timer::Register::TIMA], 0);
}

/**
 * @brief Verifies that writing TAC with bit 2 clear disables the timer.
 * @details TAC = 0x01 → clock mode 01 selected but bit 2 = 0 → _cycle_tima = 0 → TIMA never
 *          increments regardless of elapsed cycles.
 */
TEST_F(TimerTest, TIMA_StaysZeroWithTAC_TimerBitClear)
{
    ram.write(Timer::Register::TAC, 0x01); // clock mode 01 selected, but timer disabled
    timer.step(ram, cpu_freq / 262144 * 10);
    EXPECT_EQ(ram[Timer::Register::TIMA], 0);
}

// ---------------------------------------------------------------------------
// TIMA — clock frequency selection
// ---------------------------------------------------------------------------

/**
 * @brief Verifies TIMA increments once after one period at 4096 Hz (TAC = 0x04).
 * @details cpu_freq / 4096 = 1024 cycles per tick.
 */
TEST_F(TimerTest, TIMA_Increment_4096Hz)
{
    ram.write(Timer::Register::TAC, 0x04);
    timer.step(ram, cpu_freq / 4096);
    EXPECT_EQ(ram[Timer::Register::TIMA], 1);
}

/**
 * @brief Verifies TIMA increments once after one period at 262144 Hz (TAC = 0x05).
 * @details cpu_freq / 262144 = 16 cycles per tick.
 */
TEST_F(TimerTest, TIMA_Increment_262144Hz)
{
    ram.write(Timer::Register::TAC, 0x05);
    timer.step(ram, cpu_freq / 262144);
    EXPECT_EQ(ram[Timer::Register::TIMA], 1);
}

/**
 * @brief Verifies TIMA increments once after one period at 65536 Hz (TAC = 0x06).
 * @details cpu_freq / 65536 = 64 cycles per tick.
 */
TEST_F(TimerTest, TIMA_Increment_65536Hz)
{
    ram.write(Timer::Register::TAC, 0x06);
    timer.step(ram, cpu_freq / 65536);
    EXPECT_EQ(ram[Timer::Register::TIMA], 1);
}

/**
 * @brief Verifies TIMA increments once after one period at 16384 Hz (TAC = 0x07).
 * @details cpu_freq / 16384 = 256 cycles per tick.
 */
TEST_F(TimerTest, TIMA_Increment_16384Hz)
{
    ram.write(Timer::Register::TAC, 0x07);
    timer.step(ram, cpu_freq / 16384);
    EXPECT_EQ(ram[Timer::Register::TIMA], 1);
}

/**
 * @brief Verifies TIMA increments multiple times within a single step() call.
 * @details At 4096 Hz, stepping 5 full periods in one call exercises the while loop and
 *          produces TIMA = 5.
 */
TEST_F(TimerTest, TIMA_MultipleIncrements_SingleStep)
{
    ram.write(Timer::Register::TAC, 0x04);
    timer.step(ram, (cpu_freq / 4096) * 5);
    EXPECT_EQ(ram[Timer::Register::TIMA], 5);
}

// ---------------------------------------------------------------------------
// TIMA — overflow boundary
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that incrementing TIMA from 0xFE to 0xFF does not set the TIMER interrupt.
 * @details 0xFF is the maximum value, not yet an overflow. The interrupt fires only on the
 *          next tick that would push TIMA past 0xFF.
 */
TEST_F(TimerTest, TIMA_NoInterruptAtBoundary_0xFE_To_0xFF)
{
    ram.write_register(Timer::Register::TIMA, 0xFE);
    ram.write(Timer::Register::TAC, 0x04);
    timer.step(ram, cpu_freq / 4096);
    EXPECT_EQ(ram[Timer::Register::TIMA], 0xFF);
    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::TIMER, 0);
}

/**
 * @brief Verifies that TIMA overflowing 0xFF loads the value from TMA.
 * @details TIMA is manually set to 0xFF. After one period, the overflow path fires:
 *          TIMA is replaced by TMA (0x42).
 */
TEST_F(TimerTest, TIMA_OverflowLoadsFromTMA)
{
    ram.write_register(Timer::Register::TIMA, 0xFF);
    ram.write_register(Timer::Register::TMA, 0x42);
    ram.write(Timer::Register::TAC, 0x04);
    timer.step(ram, cpu_freq / 4096);
    EXPECT_EQ(ram[Timer::Register::TIMA], 0x42);
}

/**
 * @brief Verifies that TIMA overflow sets the TIMER flag (bit 2) in the IF register.
 */
TEST_F(TimerTest, TIMA_OverflowSetsTimerInterrupt)
{
    ram.write_register(Timer::Register::TIMA, 0xFF);
    ram.write_register(Timer::Register::TMA, 0x00);
    ram.write(Timer::Register::TAC, 0x04);
    timer.step(ram, cpu_freq / 4096);
    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::TIMER, CPU::IFFlag::TIMER);
}

/**
 * @brief Verifies that the pre-existing IF flags are preserved when the TIMER interrupt fires.
 * @details IF = 0x03 (VBLANK + LCD) before overflow. After overflow, IF must be 0x03 | TIMER.
 */
TEST_F(TimerTest, TIMA_OverflowPreservesOtherIFFlags)
{
    ram.write_register(CPU::Register::IF, 0x03);
    ram.write_register(Timer::Register::TIMA, 0xFF);
    ram.write_register(Timer::Register::TMA, 0x00);
    ram.write(Timer::Register::TAC, 0x04);
    timer.step(ram, cpu_freq / 4096);
    EXPECT_EQ(ram[CPU::Register::IF], 0x03 | CPU::IFFlag::TIMER);
}

/**
 * @brief Verifies that after an overflow, TIMA continues counting from TMA in the same step().
 * @details TIMA = 0xFF, TMA = 0x00. Stepping 2 periods fires the overflow (TIMA ← TMA = 0),
 *          then the second tick increments TIMA to 1. Exercises the while loop continuing
 *          across the reload boundary.
 */
TEST_F(TimerTest, TIMA_OverflowThenContinuesFromTMA)
{
    ram.write_register(Timer::Register::TIMA, 0xFF);
    ram.write_register(Timer::Register::TMA, 0x00);
    ram.write(Timer::Register::TAC, 0x04); // 4096 Hz, period = 1024

    timer.step(ram, (cpu_freq / 4096) * 2);

    EXPECT_EQ(ram[Timer::Register::TIMA], 1);
    EXPECT_EQ(ram[CPU::Register::IF] & CPU::IFFlag::TIMER, CPU::IFFlag::TIMER);
}

// ---------------------------------------------------------------------------
// TIMA — enabling / disabling the timer
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that the timer can be disabled mid-operation by writing TAC bit 2 = 0.
 * @details Enable at 4096 Hz, let TIMA reach 3, then disable with bit 2 = 0 while keeping
 *          the same clock mode (0x01). Further steps must not change TIMA.
 */
TEST_F(TimerTest, TIMA_StopsAfterTimerDisabled)
{
    ram.write(Timer::Register::TAC, 0x04); // enable, clock mode 00 (4096 Hz)
    timer.step(ram, (cpu_freq / 4096) * 3);
    ASSERT_EQ(ram[Timer::Register::TIMA], 3);

    ram.write(Timer::Register::TAC, 0x00); // disable, keep same clock mode
    timer.step(ram, (cpu_freq / 4096) * 10);
    EXPECT_EQ(ram[Timer::Register::TIMA], 3);
}

/**
 * @brief Verifies that switching clock frequency via TAC correctly changes the tick period.
 * @details Start at 4096 Hz for one tick, then switch to 16384 Hz. One additional tick at
 *          the new rate should advance TIMA by 1.
 */
TEST_F(TimerTest, TIMA_ClockSwitchChangesPeriod)
{
    ram.write(Timer::Register::TAC, 0x04); // 4096 Hz, period = 1024
    timer.step(ram, cpu_freq / 4096);
    ASSERT_EQ(ram[Timer::Register::TIMA], 1);

    ram.write(Timer::Register::TAC, 0x07); // 16384 Hz, period = 256
    timer.step(ram, cpu_freq / 16384);
    EXPECT_EQ(ram[Timer::Register::TIMA], 2);
}

// ---------------------------------------------------------------------------
// ISerializable — save_state / load_state
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that save_state / load_state correctly preserves the DIV internal counter.
 * @details After stepping half a DIV period (128 cycles), _next_cycle_div = 128 and DIV is
 *          still 0. The state is saved and restored on a fresh Timer. Stepping the remaining
 *          128 cycles must trigger exactly one DIV increment, proving the counter was restored.
 */
TEST_F(TimerTest, SaveLoadState_PreservesDivTiming)
{
    timer.step(ram, cycles_per_div_increment / 2); // 128 cycles elapsed
    ASSERT_EQ(ram[Timer::Register::DIV], 0);       // not yet incremented

    StateMap state;
    timer.save_state(state);

    Timer timer2{};
    timer2.init(ram);
    timer2.load_state(state);

    // Remaining 128 cycles complete the period → exactly one DIV increment
    timer2.step(ram, cycles_per_div_increment / 2);
    EXPECT_EQ(ram[Timer::Register::DIV], 1);
}

/**
 * @brief Verifies that save_state / load_state correctly preserves the TIMA internal counter.
 * @details Enable 4096 Hz (period = 1024). Step 512 cycles — half the period — so TIMA stays
 *          at 0. Save and restore on a fresh Timer. Stepping the remaining 512 cycles must
 *          fire exactly one TIMA increment, proving _next_cycle_tima and _cycle_tima were
 *          restored.
 */
TEST_F(TimerTest, SaveLoadState_PreservesTimaTiming)
{
    ram.write(Timer::Register::TAC, 0x04); // 4096 Hz, period = 1024
    timer.step(ram, cpu_freq / 4096 / 2); // 512 cycles — half a TIMA period
    ASSERT_EQ(ram[Timer::Register::TIMA], 0);

    StateMap state;
    timer.save_state(state);

    Timer timer2{};
    timer2.init(ram);
    timer2.load_state(state);

    // Remaining 512 cycles complete the period → exactly one TIMA increment
    timer2.step(ram, cpu_freq / 4096 / 2);
    EXPECT_EQ(ram[Timer::Register::TIMA], 1);
}
