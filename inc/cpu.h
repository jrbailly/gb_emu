#ifndef _CPU_H_
#define _CPU_H_

#include "cartridge.h"
#include "ram.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>

static constexpr size_t OPCODE_SIZE = 0xFF;
static constexpr int CPU_FREQ = 4194304;

struct Registers
{
    unsigned char ime;
    unsigned short pc;
    unsigned short sp;
    union {
        unsigned char regs8[8];
        unsigned short regs16[4];
    };
};

class CPU
{

  public:
    CPU(RamBus &ram);
    void debug(uint32_t cycles);
    uint8_t step();
    void save_state(const std::string &rom_file);
    void load_state(const std::string &rom_file);
    void load_registers(const std::map<std::string, int> &registers_value);
    std::map<std::string, int> get_registers();

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

  private:
    void load_register(uint8_t reg, uint8_t value);
    void write_ram(uint16_t address, uint8_t value);
    void load_register16(uint8_t reg, uint16_t value);
    void write_ram16(uint16_t address, uint16_t value);
    void push(uint8_t reg);
    void pop(uint8_t reg);
    void add_stack(int8_t value);
    void add(uint8_t value, uint8_t carry);
    void sub(uint8_t value, uint8_t carry);
    void cp(uint8_t value);
    void inc(uint8_t value);
    void inc_hl();
    void dec(uint8_t value);
    void dec_hl();
    void and_(uint8_t value);
    void or_(uint8_t value);
    void xor_(uint8_t value);
    void ccf();
    void scf();
    void daa();
    void cpl();
    void inc16(uint8_t reg);
    void dec16(uint8_t reg);
    void add_hl(uint16_t value);
    void add_sp(int8_t reg);
    uint8_t rotl(uint8_t value, bool zflag);
    uint8_t rotlc(uint8_t value, bool zflag);
    uint8_t rotr(uint8_t value, bool zflag);
    uint8_t rotrc(uint8_t value, bool zflag);
    uint8_t shiftl(uint8_t value);
    uint8_t shiftr(uint8_t value);
    uint8_t shiftr2(uint8_t value);
    uint8_t swap(uint8_t value);
    void bit_test(uint8_t bit, uint8_t value);
    uint8_t bit_set(uint8_t bit, uint8_t value);
    uint8_t bit_reset(uint8_t bit, uint8_t value);
    void jump(uint16_t addr);
    void jump_conditionnal(uint8_t opcode, uint16_t addr);
    void call(uint16_t addr);
    void call_conditionnal(uint8_t opcode, uint16_t addr);
    void ret();
    bool ret_conditionnal(uint8_t opcode);
    // miscellaneous instructions
    void HALT();
    void STOP();
    void di();
    void ei();
    void nop();
    uint8_t active_interrupt(uint16_t addr);
    uint8_t decode();
    uint8_t decodeExtendOpcode();

  private:
    Registers mRegister;
    RamBus &mRAM;
    std::map<std::string, int> mRegisterIndex;
    std::array<Reg8, 8> mMapReg;
};

#endif