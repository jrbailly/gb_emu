#ifndef _CPU_H_
#define _CPU_H_

#include "cartridge.h"
#include "iserializable.h"
#include "ram.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>

static constexpr size_t opcode_size = 0xFF;
static constexpr int cpu_freq = 4194304;
static constexpr int machine_cycle = 4;

struct Registers
{
    unsigned char halt;
    unsigned char ime;
    unsigned short pc;
    unsigned short sp;
    union {
        unsigned char regs8[8];
        unsigned short regs16[4];
    };
};

class CPU : public ISerializable
{

  public:
    CPU(RamBus &ram);
    auto debug(uint32_t cycles) -> void;
    auto step() -> uint8_t;
    auto save_state(StateMap &state) -> void override;
    auto load_state(const StateMap &state) -> void override;

  public:
    enum Register
    {
        IF = 0xFF0F,
        IE = 0xFFFF,
    };
    enum Reg8
    {
        C = 0,
        B,
        E,
        D,
        L,
        H,
        F,
        A
    };

    enum Reg16
    {
        BC = 0,
        DE,
        HL,
        AF
    };
    enum Flags
    {
        c = 0x10,
        h = 0x20,
        n = 0x40,
        z = 0x80
    };
    enum InterruptAddress
    {
        VBLANK_ADDR = 0x40,
        STAT_ADDR = 0x48,
        TIMER_ADDR = 0x50,
        SERIAL_ADDR = 0x58,
        JOYPAD_ADDR = 0x60,
    };

    enum IFFlag
    {
        VBLANK = 0x01,
        LCD = 0x02,
        TIMER = 0x04,
        SERIAL = 0x08,
        JOYPAD = 0x10,
    };

  private:
    auto load_register(uint8_t reg, uint8_t value) -> void;
    auto write_ram(uint16_t address, uint8_t value) -> void;
    auto load_register16(uint8_t reg, uint16_t value) -> void;
    auto write_ram16(uint16_t address, uint16_t value) -> void;
    auto push(uint8_t reg) -> void;
    auto pop(uint8_t reg) -> void;
    auto add_stack(int8_t value) -> void;
    auto add(uint8_t value, uint8_t carry) -> void;
    auto sub(uint8_t value, uint8_t carry) -> void;
    auto cp(uint8_t value) -> void;
    auto inc(uint8_t value) -> void;
    auto inc_hl() -> void;
    auto dec(uint8_t value) -> void;
    auto dec_hl() -> void;
    auto and_(uint8_t value) -> void;
    auto or_(uint8_t value) -> void;
    auto xor_(uint8_t value) -> void;
    auto ccf() -> void;
    auto scf() -> void;
    auto daa() -> void;
    auto cpl() -> void;
    auto inc16(uint8_t reg) -> void;
    auto dec16(uint8_t reg) -> void;
    auto add_hl(uint16_t value) -> void;
    auto add_sp(int8_t reg) -> void;
    auto rotl(uint8_t value, bool zflag) -> uint8_t;
    auto rotlc(uint8_t value, bool zflag) -> uint8_t;
    auto rotr(uint8_t value, bool zflag) -> uint8_t;
    auto rotrc(uint8_t value, bool zflag) -> uint8_t;
    auto shiftl(uint8_t value) -> uint8_t;
    auto shiftr(uint8_t value) -> uint8_t;
    auto shiftr2(uint8_t value) -> uint8_t;
    auto swap(uint8_t value) -> uint8_t;
    auto bit_test(uint8_t bit, uint8_t value) -> void;
    auto bit_set(uint8_t bit, uint8_t value) -> uint8_t;
    auto bit_reset(uint8_t bit, uint8_t value) -> uint8_t;
    auto jump(uint16_t addr) -> void;
    auto jump_conditionnal(uint8_t opcode, uint16_t addr) -> bool;
    auto call(uint16_t addr) -> void;
    auto call_conditionnal(uint8_t opcode, uint16_t addr) -> void;
    auto ret() -> void;
    auto reti() -> void;
    auto ret_conditionnal(uint8_t opcode) -> bool;
    auto halt() -> void;
    auto stop() -> void;
    auto di() -> void;
    auto ei() -> void;
    auto nop() -> void;
    auto active_interrupt(uint8_t bit, uint16_t addr) -> uint8_t;
    auto decode() -> uint8_t;
    auto decodeExtendOpcode() -> uint8_t;

  private:
    Registers _registers;
    RamBus &_ram;
    bool _active_interruption;
    std::map<std::string, int> _register_index;
    std::array<Reg8, 8> _map_reg;
};

#endif