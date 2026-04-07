#include "cpu.h"
#include "timer.h"
#include <array>
#include <cstdint>
#include <fstream>

constexpr std::array<uint16_t, 2048> intToBcd = [] {
    std::array<uint16_t, 2048> arr;

    for (int i = 0; i < 2048; ++i)
    {
        uint8_t n = (i >> 10) & 1;
        uint8_t h = (i >> 9) & 1;
        uint8_t c = (i >> 8) & 1;
        uint8_t val = i & 0xFF;
        uint8_t carry = 0;

        // note: assumes a is a uint8_t and wraps from 0xff to 0
        if (n == 0)
        { // after an addition, adjust if (half-)carry occurred or if result is out of bounds
            if (c == 1 || val > 0x99)
            {
                val += 0x60;
                carry = 1;
            }
            if (h == 1 || (val & 0x0f) > 0x09)
                val += 0x6;
        }
        else
        { // after a subtraction, only adjust if (half-)carry occurred
            if (c)
                val -= 0x60;
            if (h)
                val -= 0x6;
            carry = c;
        }
        arr[i] = val;
        if (carry == 1)
            arr[i] |= 0x200;
    }
    return arr;
}();

/**
 * @brief Constructs a CPU object with a reference to the RAM bus.
 * @param ram Reference to the RAM bus.
 */
CPU::CPU(RamBus &ram) : _ram(ram), _active_interruption(false)
{
    _registers.halt = 0;
    _registers.ime = 0;
    _registers.pc = 0x100;
    _registers.sp = 0xfffe;
    _registers.regs8[Reg8::A] = 0x11;
    _registers.regs8[Reg8::F] = 0xB0;
    _registers.regs8[Reg8::B] = 0x00;
    _registers.regs8[Reg8::C] = 0x13;
    _registers.regs8[Reg8::D] = 0;
    _registers.regs8[Reg8::E] = 0xd8;
    _registers.regs8[Reg8::H] = 0x01;
    _registers.regs8[Reg8::L] = 0x4d;
    _register_index = {{"b", Reg8::B}, {"c", Reg8::C}, {"d", Reg8::D}, {"e", Reg8::E},
                       {"h", Reg8::H}, {"l", Reg8::L}, {"f", Reg8::F}, {"a", Reg8::A}};
    _map_reg = {Reg8::B, Reg8::C, Reg8::D, Reg8::E, Reg8::H, Reg8::L, Reg8::F, Reg8::A};
}

auto CPU::save_state(StateMap &state) -> void
{
    for (auto &reg : _register_index)
        state[reg.first] = _registers.regs8[reg.second];
    state["pc"] = _registers.pc;
    state["sp"] = _registers.sp;
    state["ime"] = _registers.ime;
    state["hlt"] = _registers.halt;
    state["interrupt"] = _active_interruption;
}

auto CPU::load_state(const StateMap &state) -> void
{
    for (auto &item : state)
        if (_register_index.contains(item.first))
            _registers.regs8[_register_index[item.first]] = std::get<int>(item.second);
    _registers.pc = std::get<int>(state.at("pc"));
    _registers.sp = std::get<int>(state.at("sp"));
    _registers.ime = std::get<int>(state.at("ime"));
    _registers.halt = std::get<int>(state.at("hlt"));
    _active_interruption = std::get<int>(state.at("interrupt"));
}

/**
 * @brief Outputs debug information about the CPU state.
 * @param cycles Number of executed cycles.
 */
auto CPU::debug(uint32_t cycles) -> void
{
    if (_registers.halt == 0)
    {
        FILE *f = stdout;

        fprintf(f, "%04X:", _registers.pc);
        fprintf(f,
                " A:%02x F:%02x B:%02x C:%02x D:%02x E:%02x H:%02x L:%02x LY:%02x SP:%04x  (Cy: %d) IF:%02x IE:%02x "
                "LY:%d\n",
                _registers.regs8[Reg8::A], _registers.regs8[Reg8::F], _registers.regs8[Reg8::B],
                _registers.regs8[Reg8::C], _registers.regs8[Reg8::D], _registers.regs8[Reg8::E],
                _registers.regs8[Reg8::H], _registers.regs8[Reg8::L], _ram[0xFF44], _registers.sp, cycles,
                _ram[Register::IF], _ram[Register::IE], (int)_ram[0xFF44]);
    }
}

/**
 * @brief Executes one CPU instruction and returns the number of cycles taken.
 * @return The number of cycles taken by the instruction.
 */
auto CPU::step() -> uint8_t
{
    int8_t cycles_count = 1;
    bool actived_interrupt = false;

    if (_registers.halt && (_ram[Register::IF] & _ram[Register::IE]))
        _registers.halt = 0;
    if (_registers.ime == 1 && _active_interruption == false)
    {
        uint8_t interrupts = _ram[Register::IE] & _ram[Register::IF];
        if (interrupts & IFFlag::VBLANK)
            cycles_count = active_interrupt(IFFlag::VBLANK, InterruptAddress::VBLANK_ADDR);
        else if (interrupts & IFFlag::LCD)
            cycles_count = active_interrupt(IFFlag::LCD, InterruptAddress::STAT_ADDR);
        else if (interrupts & IFFlag::TIMER)
            cycles_count = active_interrupt(IFFlag::TIMER, InterruptAddress::TIMER_ADDR);
        else if (interrupts & IFFlag::SERIAL)
            cycles_count = active_interrupt(IFFlag::SERIAL, InterruptAddress::SERIAL_ADDR);
        else if (interrupts & IFFlag::JOYPAD)
            cycles_count = active_interrupt(IFFlag::JOYPAD, InterruptAddress::JOYPAD_ADDR);
        if (interrupts)
            actived_interrupt = true;
    }
    _active_interruption = false;
    if (actived_interrupt == false && _registers.halt == 0)
        cycles_count = decode();
    return (cycles_count * machine_cycle);
}

/**
 * @brief Decodes and executes the next instruction.
 * @return The number of cycles taken by the instruction.
 */
auto CPU::decode() -> uint8_t
{
    uint8_t opcode = _ram[_registers.pc++];
    uint8_t reg = _map_reg[(opcode >> 3) & 0x7];
    uint16_t address;
    uint16_t value;
    uint16_t carry = (_registers.regs8[Reg8::F] >> 4) & 1;
    int8_t relative;
    int8_t cycles_count = 0;
    bool result;

    switch (opcode)
    {
    case (0x40):
    case (0x41):
    case (0x42):
    case (0x43):
    case (0x44):
    case (0x45):
    case (0x47):
    case (0x48):
    case (0x49):
    case (0x4a):
    case (0x4b):
    case (0x4c):
    case (0x4d):
    case (0x4f):
    case (0x50):
    case (0x51):
    case (0x52):
    case (0x53):
    case (0x54):
    case (0x55):
    case (0x57):
    case (0x58):
    case (0x59):
    case (0x5a):
    case (0x5b):
    case (0x5c):
    case (0x5d):
    case (0x5f):
    case (0x60):
    case (0x61):
    case (0x62):
    case (0x63):
    case (0x64):
    case (0x65):
    case (0x67):
    case (0x68):
    case (0x69):
    case (0x6a):
    case (0x6b):
    case (0x6c):
    case (0x6d):
    case (0x6f):
    case (0x78):
    case (0x79):
    case (0x7a):
    case (0x7b):
    case (0x7c):
    case (0x7d):
    case (0x7f):
        load_register(reg, _registers.regs8[_map_reg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0x06):
    case (0x0e):
    case (0x16):
    case (0x1e):
    case (0x26):
    case (0x2e):
    case (0x3e):
        load_register(reg, _ram[_registers.pc++]);
        cycles_count = 2;
        break;
    case (0x46):
    case (0x4e):
    case (0x56):
    case (0x5e):
    case (0x66):
    case (0x6e):
    case (0x7e):
        load_register(reg, _ram[_registers.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0x70):
    case (0x71):
    case (0x72):
    case (0x73):
    case (0x74):
    case (0x75):
    case (0x77):
        write_ram(_registers.regs16[Reg16::HL], _registers.regs8[_map_reg[opcode & 0x7]]);
        cycles_count = 2;
        break;
    case (0x36):
        write_ram(_registers.regs16[Reg16::HL], _ram[_registers.pc++]);
        cycles_count = 3;
        break;
    case (0x0a):
        load_register(Reg8::A, _ram[_registers.regs16[Reg16::BC]]);
        cycles_count = 2;
        break;
    case (0x1a):
        load_register(Reg8::A, _ram[_registers.regs16[Reg16::DE]]);
        cycles_count = 2;
        break;
    case (0xfa):
        value = _ram[_registers.pc++];
        value |= _ram[_registers.pc++] << 8;
        load_register(Reg8::A, _ram[value]);
        cycles_count = 4;
        break;
    case (0x3a):
        load_register(Reg8::A, _ram[_registers.regs16[Reg16::HL]]);
        _registers.regs16[Reg16::HL]--;
        cycles_count = 2;
        break;
    case (0x2a):
        load_register(Reg8::A, _ram[_registers.regs16[Reg16::HL]]);
        _registers.regs16[Reg16::HL]++;
        cycles_count = 2;
        break;
    case (0xf2):
        address = 0xFF00 | _registers.regs8[Reg8::C];
        load_register(Reg8::A, _ram[address]);
        cycles_count = 2;
        break;
    case (0xf0):
        address = 0xFF00 | _ram[_registers.pc++];
        load_register(Reg8::A, _ram[address]);
        cycles_count = 3;
        break;
    case (0x02):
        write_ram(_registers.regs16[Reg16::BC], _registers.regs8[Reg8::A]);
        cycles_count = 2;
        break;
    case (0x12):
        write_ram(_registers.regs16[Reg16::DE], _registers.regs8[Reg8::A]);
        cycles_count = 2;
        break;
    case (0xea):
        value = _ram[_registers.pc++];
        value |= _ram[_registers.pc++] << 8;
        write_ram(value, _registers.regs8[Reg8::A]);
        cycles_count = 4;
        break;
    case (0xe2):
        address = 0xFF00 | _registers.regs8[Reg8::C];
        write_ram(address, _registers.regs8[Reg8::A]);
        cycles_count = 2;
        break;
    case (0xe0):
        address = 0xFF00 | _ram[_registers.pc++];
        write_ram(address, _registers.regs8[Reg8::A]);
        cycles_count = 3;
        break;
    case (0x32):
        write_ram(_registers.regs16[Reg16::HL], _registers.regs8[Reg8::A]);
        _registers.regs16[Reg16::HL]--;
        cycles_count = 2;
        break;
    case (0x22):
        write_ram(_registers.regs16[Reg16::HL], _registers.regs8[Reg8::A]);
        _registers.regs16[Reg16::HL]++;
        cycles_count = 2;
        break;
    case (0x01):
    case (0x11):
    case (0x21):
        reg = (opcode >> 4) & 0x3;
        value = _ram[_registers.pc++];
        value |= (_ram[_registers.pc++] << 8);
        load_register16(reg, value);
        cycles_count = 3;
        break;
    case (0x31):
        value = _ram[_registers.pc++];
        value |= (_ram[_registers.pc++] << 8);
        _registers.sp = value;
        cycles_count = 3;
        break;
    case (0x08):
        address = _ram[_registers.pc++];
        address |= (_ram[_registers.pc++] << 8);
        write_ram16(address, _registers.sp);
        cycles_count = 5;
        break;
    case (0xf9):
        _registers.sp = _registers.regs16[Reg16::HL];
        cycles_count = 2;
        break;
    case (0xc5):
    case (0xd5):
    case (0xe5):
    case (0xf5):
        push((opcode >> 4) & 0x3);
        cycles_count = 4;
        break;
    case (0xc1):
    case (0xd1):
    case (0xe1):
    case (0xf1):
        pop((opcode >> 4) & 0x3);
        cycles_count = 3;
        break;
    case (0xf8):
        add_stack(_ram[_registers.pc++]);
        cycles_count = 3;
        break;
    case (0x80):
    case (0x81):
    case (0x82):
    case (0x83):
    case (0x84):
    case (0x85):
    case (0x87):
        add(_registers.regs8[_map_reg[opcode & 0x7]], 0);
        cycles_count = 1;
        break;
    case (0x86):
        add(_ram[_registers.regs16[Reg16::HL]], 0);
        cycles_count = 2;
        break;
    case (0xc6):
        add(_ram[_registers.pc++], 0);
        cycles_count = 2;
        break;
    case (0x88):
    case (0x89):
    case (0x8a):
    case (0x8b):
    case (0x8c):
    case (0x8d):
    case (0x8f):
        add(_registers.regs8[_map_reg[opcode & 0x7]], carry);
        cycles_count = 1;
        break;
    case (0x8e):
        add(_ram[_registers.regs16[Reg16::HL]], carry);
        cycles_count = 2;
        break;
    case (0xce):
        add(_ram[_registers.pc++], carry);
        cycles_count = 2;
        break;
    case (0x90):
    case (0x91):
    case (0x92):
    case (0x93):
    case (0x94):
    case (0x95):
    case (0x97):
        sub(_registers.regs8[_map_reg[opcode & 0x7]], 0);
        cycles_count = 1;
        break;
    case (0x96):
        sub(_ram[_registers.regs16[Reg16::HL]], 0);
        cycles_count = 2;
        break;
    case (0xd6):
        sub(_ram[_registers.pc++], 0);
        cycles_count = 2;
        break;
    case (0x98):
    case (0x99):
    case (0x9a):
    case (0x9b):
    case (0x9c):
    case (0x9d):
    case (0x9f):
        sub(_registers.regs8[_map_reg[opcode & 0x7]], carry);
        cycles_count = 1;
        break;
    case (0x9e):
        sub(_ram[_registers.regs16[Reg16::HL]], carry);
        cycles_count = 2;
        break;
    case (0xde):
        sub(_ram[_registers.pc++], carry);
        cycles_count = 2;
        break;
    case (0xb8):
    case (0xb9):
    case (0xba):
    case (0xbb):
    case (0xbc):
    case (0xbd):
    case (0xbf):
        cp(_registers.regs8[_map_reg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xbe):
        cp(_ram[_registers.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xfe):
        cp(_ram[_registers.pc++]);
        cycles_count = 2;
        break;
    case (0x04):
    case (0x0c):
    case (0x14):
    case (0x1c):
    case (0x24):
    case (0x2c):
    case (0x3c):
        inc(reg);
        cycles_count = 1;
        break;
    case (0x34):
        inc_hl();
        cycles_count = 3;
        break;
    case (0x05):
    case (0x0d):
    case (0x15):
    case (0x1d):
    case (0x25):
    case (0x2d):
    case (0x3d):
        dec(reg);
        cycles_count = 1;
        break;
    case (0x35):
        dec_hl();
        cycles_count = 3;
        break;
    case (0xa0):
    case (0xa1):
    case (0xa2):
    case (0xa3):
    case (0xa4):
    case (0xa5):
    case (0xa7):
        and_(_registers.regs8[_map_reg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xa6):
        and_(_ram[_registers.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xe6):
        and_(_ram[_registers.pc++]);
        cycles_count = 2;
        break;
    case (0xb0):
    case (0xb1):
    case (0xb2):
    case (0xb3):
    case (0xb4):
    case (0xb5):
    case (0xb7):
        or_(_registers.regs8[_map_reg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xb6):
        or_(_ram[_registers.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xf6):
        or_(_ram[_registers.pc++]);
        cycles_count = 2;
        break;
    case (0xa8):
    case (0xa9):
    case (0xaa):
    case (0xab):
    case (0xac):
    case (0xad):
    case (0xaf):
        xor_(_registers.regs8[_map_reg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xae):
        xor_(_ram[_registers.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xee):
        xor_(_ram[_registers.pc++]);
        cycles_count = 2;
        break;
    case (0x3f):
        ccf();
        cycles_count = 1;
        break;
    case (0x37):
        scf();
        cycles_count = 1;
        break;
    case (0x27):
        daa();
        cycles_count = 1;
        break;
    case (0x2f):
        cycles_count = 1;
        cpl();
        break;
    case (0x03):
    case (0x13):
    case (0x23):
        inc16((opcode >> 4) & 0x3);
        cycles_count = 2;
        break;
    case (0x33):
        _registers.sp++;
        cycles_count = 2;
        break;
    case (0x0b):
    case (0x1b):
    case (0x2b):
        dec16((opcode >> 4) & 0x3);
        cycles_count = 2;
        break;
    case (0x3b):
        _registers.sp--;
        cycles_count = 2;
        break;
    case (0x09):
    case (0x19):
    case (0x29):
        add_hl(_registers.regs16[(opcode >> 4) & 0x3]);
        cycles_count = 2;
        break;
    case (0x39):
        add_hl(_registers.sp);
        cycles_count = 2;
        break;
    case (0xe8):
        add_sp(_ram[_registers.pc++]);
        cycles_count = 4;
        break;
    case (0x07):
        _registers.regs8[Reg8::A] = rotl(_registers.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0x0f):
        _registers.regs8[Reg8::A] = rotr(_registers.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0x17):
        _registers.regs8[Reg8::A] = rotlc(_registers.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0x1f):
        _registers.regs8[Reg8::A] = rotrc(_registers.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0xc3):
        value = _ram[_registers.pc++];
        value |= (_ram[_registers.pc++] << 8);
        jump(value);
        cycles_count = 4;
        break;
    case (0xe9):
        jump(_registers.regs16[Reg16::HL]);
        cycles_count = 1;
        break;
    case (0xc2):
    case (0xca):
    case (0xd2):
    case (0xda):
        cycles_count = 4;
        value = _ram[_registers.pc++];
        value |= (_ram[_registers.pc++] << 8);
        if (!jump_conditionnal(opcode, value))
            cycles_count--;
        break;
    case (0x18):
        relative = _ram[_registers.pc++];
        jump(_registers.pc + relative);
        cycles_count = 3;
        break;
    case (0x20):
    case (0x28):
    case (0x30):
    case (0x38):
        cycles_count = 3;
        relative = _ram[_registers.pc++];
        value = _registers.pc + relative;
        if (!jump_conditionnal(opcode, value))
            cycles_count--;
        break;
    case (0xcd):
        value = _ram[_registers.pc++];
        value |= (_ram[_registers.pc++] << 8);
        call(value);
        cycles_count = 6;
        break;
    case (0xc4):
    case (0xcc):
    case (0xd4):
    case (0xdc):
        value = _ram[_registers.pc++];
        value |= (_ram[_registers.pc++] << 8);
        call_conditionnal(opcode, value);
        cycles_count = 6;
        if (_registers.pc != value)
            cycles_count = 3;
        break;
    case (0xc9):
        ret();
        cycles_count = 4;
        break;
    case (0xc0):
    case (0xc8):
    case (0xd0):
    case (0xd8):
        result = ret_conditionnal(opcode);
        cycles_count = 5;
        if (!result)
            cycles_count = 2;
        break;
    case (0xd9):
        reti();
        cycles_count = 4;
        break;
    case (0xc7):
    case (0xcf):
    case (0xd7):
    case (0xdf):
    case (0xe7):
    case (0xef):
    case (0xf7):
    case (0xff):
        value = opcode & 0x38;
        call(value);
        cycles_count = 4;
        break;
    case (0x76):
        halt();
        cycles_count = 1;
        break;
    case (0x10):
        stop();
        cycles_count = 1;
        break;
    case (0xf3):
        di();
        cycles_count = 1;
        break;
    case (0xfb):
        ei();
        cycles_count = 1;
        break;
    case (0x00):
        nop();
        cycles_count = 1;
        break;
    case (0xcb):
        cycles_count = decodeExtendOpcode();
        break;
    }
    return (cycles_count);
}

/**
 * @brief Decodes and executes an extended opcode instruction.
 * @return The number of cycles taken by the instruction.
 */
inline auto CPU::decodeExtendOpcode() -> uint8_t
{
    uint8_t opcode = _ram[_registers.pc++];
    uint16_t address;
    uint8_t bit;
    uint8_t reg = _map_reg[opcode & 0x7];
    uint8_t value;
    uint8_t cycles_count = 0;

    switch (opcode)
    {
    case (0x00):
    case (0x01):
    case (0x02):
    case (0x03):
    case (0x04):
    case (0x05):
    case (0x07):
        _registers.regs8[reg] = rotl(_registers.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x06):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, rotl(value, true));
        cycles_count = 4;
        break;
    case (0x08):
    case (0x09):
    case (0x0a):
    case (0x0b):
    case (0x0c):
    case (0x0d):
    case (0x0f):
        _registers.regs8[reg] = rotr(_registers.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x0e):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, rotr(value, true));
        cycles_count = 4;
        break;
    case (0x10):
    case (0x11):
    case (0x12):
    case (0x13):
    case (0x14):
    case (0x15):
    case (0x17):
        _registers.regs8[reg] = rotlc(_registers.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x16):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, rotlc(value, true));
        cycles_count = 4;
        break;
    case (0x18):
    case (0x19):
    case (0x1a):
    case (0x1b):
    case (0x1c):
    case (0x1d):
    case (0x1f):
        _registers.regs8[reg] = rotrc(_registers.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x1e):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, rotrc(value, true));
        cycles_count = 4;
        break;
    case (0x20):
    case (0x21):
    case (0x22):
    case (0x23):
    case (0x24):
    case (0x25):
    case (0x27):
        _registers.regs8[reg] = shiftl(_registers.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x26):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, shiftl(value));
        cycles_count = 4;
        break;
    case (0x28):
    case (0x29):
    case (0x2a):
    case (0x2b):
    case (0x2c):
    case (0x2d):
    case (0x2f):
        _registers.regs8[reg] = shiftr2(_registers.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x2e):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, shiftr2(value));
        cycles_count = 4;
        break;
    case (0x30):
    case (0x31):
    case (0x32):
    case (0x33):
    case (0x34):
    case (0x35):
    case (0x37):
        _registers.regs8[reg] = swap(_registers.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x36):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, swap(value));
        cycles_count = 4;
        break;
    case (0x38):
    case (0x39):
    case (0x3a):
    case (0x3b):
    case (0x3c):
    case (0x3d):
    case (0x3f):
        _registers.regs8[reg] = shiftr(_registers.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x3e):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        write_ram(address, shiftr(value));
        cycles_count = 4;
        break;
    case (0x40):
    case (0x41):
    case (0x42):
    case (0x43):
    case (0x44):
    case (0x45):
    case (0x47):
    case (0x48):
    case (0x49):
    case (0x4a):
    case (0x4b):
    case (0x4c):
    case (0x4d):
    case (0x4f):
    case (0x50):
    case (0x51):
    case (0x52):
    case (0x53):
    case (0x54):
    case (0x55):
    case (0x57):
    case (0x58):
    case (0x59):
    case (0x5a):
    case (0x5b):
    case (0x5c):
    case (0x5d):
    case (0x5f):
    case (0x60):
    case (0x61):
    case (0x62):
    case (0x63):
    case (0x64):
    case (0x65):
    case (0x67):
    case (0x68):
    case (0x69):
    case (0x6a):
    case (0x6b):
    case (0x6c):
    case (0x6d):
    case (0x6f):
    case (0x70):
    case (0x71):
    case (0x72):
    case (0x73):
    case (0x74):
    case (0x75):
    case (0x77):
    case (0x78):
    case (0x79):
    case (0x7a):
    case (0x7b):
    case (0x7c):
    case (0x7d):
    case (0x7f):
        bit = (opcode >> 3) & 0x7;
        bit_test(bit, _registers.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x46):
    case (0x4e):
    case (0x56):
    case (0x5e):
    case (0x66):
    case (0x6e):
    case (0x76):
    case (0x7e):
        value = _ram[_registers.regs16[Reg16::HL]];
        bit = (opcode >> 3) & 0x07;
        bit_test(bit, value);
        cycles_count = 3;
        break;
    case (0x80):
    case (0x81):
    case (0x82):
    case (0x83):
    case (0x84):
    case (0x85):
    case (0x87):
    case (0x88):
    case (0x89):
    case (0x8a):
    case (0x8b):
    case (0x8c):
    case (0x8d):
    case (0x8f):
    case (0x90):
    case (0x91):
    case (0x92):
    case (0x93):
    case (0x94):
    case (0x95):
    case (0x97):
    case (0x98):
    case (0x99):
    case (0x9a):
    case (0x9b):
    case (0x9c):
    case (0x9d):
    case (0x9f):
    case (0xa0):
    case (0xa1):
    case (0xa2):
    case (0xa3):
    case (0xa4):
    case (0xa5):
    case (0xa7):
    case (0xa8):
    case (0xa9):
    case (0xaa):
    case (0xab):
    case (0xac):
    case (0xad):
    case (0xaf):
    case (0xb0):
    case (0xb1):
    case (0xb2):
    case (0xb3):
    case (0xb4):
    case (0xb5):
    case (0xb7):
    case (0xb8):
    case (0xb9):
    case (0xba):
    case (0xbb):
    case (0xbc):
    case (0xbd):
    case (0xbf):
        bit = (opcode >> 3) & 0x7;
        _registers.regs8[reg] = bit_reset(bit, _registers.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x86):
    case (0x8e):
    case (0x96):
    case (0x9e):
    case (0xa6):
    case (0xae):
    case (0xb6):
    case (0xbe):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        bit = (opcode >> 3) & 0x7;
        write_ram(address, bit_reset(bit, value));
        cycles_count = 4;
        break;
    case (0xc0):
    case (0xc1):
    case (0xc2):
    case (0xc3):
    case (0xc4):
    case (0xc5):
    case (0xc7):
    case (0xc8):
    case (0xc9):
    case (0xca):
    case (0xcb):
    case (0xcc):
    case (0xcd):
    case (0xcf):
    case (0xd0):
    case (0xd1):
    case (0xd2):
    case (0xd3):
    case (0xd4):
    case (0xd5):
    case (0xd7):
    case (0xd8):
    case (0xd9):
    case (0xda):
    case (0xdb):
    case (0xdc):
    case (0xdd):
    case (0xdf):
    case (0xe0):
    case (0xe1):
    case (0xe2):
    case (0xe3):
    case (0xe4):
    case (0xe5):
    case (0xe7):
    case (0xe8):
    case (0xe9):
    case (0xea):
    case (0xeb):
    case (0xec):
    case (0xed):
    case (0xef):
    case (0xf0):
    case (0xf1):
    case (0xf2):
    case (0xf3):
    case (0xf4):
    case (0xf5):
    case (0xf7):
    case (0xf8):
    case (0xf9):
    case (0xfa):
    case (0xfb):
    case (0xfc):
    case (0xfd):
    case (0xff):
        bit = (opcode >> 3) & 0x7;
        _registers.regs8[reg] = bit_set(bit, _registers.regs8[reg]);
        cycles_count = 2;
        break;
    case (0xc6):
    case (0xce):
    case (0xd6):
    case (0xde):
    case (0xe6):
    case (0xee):
    case (0xf6):
    case (0xfe):
        address = _registers.regs16[Reg16::HL];
        value = _ram[address];
        bit = (opcode >> 3) & 0x7;
        write_ram(address, bit_set(bit, value));
        cycles_count = 4;
        break;
    }
    return (cycles_count);
}

/**
 * @brief Loads a value into the specified 8-bit register.
 * @param reg The index of the 8-bit register to load into.
 * @param value The value to load into the register.
 */
inline auto CPU::load_register(uint8_t reg, uint8_t value) -> void
{
    _registers.regs8[reg] = value;
}

/**
 * @brief Writes a value to the specified memory address in RAM.
 * @param address The memory address to write to.
 * @param value The value to write to the memory address.
 */
inline auto CPU::write_ram(uint16_t address, uint8_t value) -> void
{
    _ram.write(address, value);
}

/**
 * @brief Loads a value into the specified 16-bit register.
 * @param reg The index of the 16-bit register to load into.
 * @param value The value to load into the register.
 */
inline auto CPU::load_register16(uint8_t reg, uint16_t value) -> void
{
    _registers.regs16[reg] = value;
}

/**
 * @brief Writes a 16-bit value to the specified memory address in RAM.
 * @param address The starting memory address to write the low byte of the value.
 * @param value The 16-bit value to write to memory.
 */
inline auto CPU::write_ram16(uint16_t address, uint16_t value) -> void
{
    _ram.write(address++, value & 0xFF);
    _ram.write(address, (value >> 8) & 0xFF);
}

/**
 * @brief Pushes the value of the specified 16-bit register onto the stack.
 * @param reg The index of the 16-bit register to push.
 */
inline auto CPU::push(uint8_t reg) -> void
{
    _registers.sp--;
    _ram.write(_registers.sp--, (_registers.regs16[reg] >> 8) & 0xFF);
    _ram.write(_registers.sp, _registers.regs16[reg] & 0xFF);
}

/**
 * @brief Pops a 16-bit value from the stack into the specified register.
 * @param reg The index of the 16-bit register to pop into.
 */
inline auto CPU::pop(uint8_t reg) -> void
{
    uint16_t Value = _ram[_registers.sp++];

    Value |= (_ram[_registers.sp++] << 8);
    _registers.regs16[reg] = Value;
    _registers.regs8[Reg8::F] &= 0xF0;
}

/**
 * @brief Adds a signed 8-bit value to the stack pointer and stores the result in HL, updating flags.
 * @param value The signed 8-bit value to add to the stack pointer.
 */
inline auto CPU::add_stack(int8_t value) -> void
{
    uint16_t sp = _registers.sp;
    uint16_t result = sp + value;
    uint16_t half_result = (sp & 0xF) + (value & 0xF);

    _registers.regs16[Reg16::HL] = result & 0xFFFF;
    _registers.regs8[Reg8::F] = 0;
    if (half_result > 0xF)
        _registers.regs8[Reg8::F] |= Flags::h;
    if ((sp & 0xFF) + static_cast<uint8_t>(value) > 0xFF)
        _registers.regs8[Reg8::F] |= Flags::c;
}

/**
 * @brief Adds a value and an optional carry to the accumulator, updating flags.
 * @param value The value to add to the accumulator.
 * @param carry The carry value to add (0 or 1).
 */
inline auto CPU::add(uint8_t value, uint8_t carry) -> void
{
    uint16_t a = _registers.regs8[Reg8::A];
    uint16_t result = a + value + carry;
    uint8_t half_result = (a & 0x0F) + (value & 0x0F) + (carry & 0x0F);

    _registers.regs8[Reg8::A] = result & 0xFF;
    _registers.regs8[Reg8::F] = 0;
    if (_registers.regs8[Reg8::A] == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if (half_result > 0x0F)
        _registers.regs8[Reg8::F] |= Flags::h;
    if (result > 0xFF)
        _registers.regs8[Reg8::F] |= Flags::c;
}

/**
 * @brief Subtracts a value and an optional carry from the accumulator, updating flags.
 * @param value The value to subtract from the accumulator.
 * @param carry The carry value to subtract (0 or 1).
 */
inline auto CPU::sub(uint8_t value, uint8_t carry) -> void
{
    uint8_t a = _registers.regs8[Reg8::A];

    _registers.regs8[Reg8::A] = a - value - carry;
    _registers.regs8[Reg8::F] = Flags::n;
    if (_registers.regs8[Reg8::A] == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if ((a & 0x0F) < ((value & 0x0F) + (carry & 0x0F)))
        _registers.regs8[Reg8::F] |= Flags::h;
    if (a < (value + carry))
        _registers.regs8[Reg8::F] |= Flags::c;
}

/**
 * @brief Compares the accumulator with a value, updating flags without storing the result.
 * @param value The value to compare with the accumulator.
 */
inline auto CPU::cp(uint8_t value) -> void
{
    uint8_t a = _registers.regs8[Reg8::A];
    uint8_t result = a - value;

    _registers.regs8[Reg8::F] = Flags::n;
    if (result == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if ((a & 0x0F) < (value & 0x0F))
        _registers.regs8[Reg8::F] |= Flags::h;
    if (a < value)
        _registers.regs8[Reg8::F] |= Flags::c;
}

/**
 * @brief Increments the specified 8-bit register and updates flags.
 * @param reg The index of the 8-bit register to increment.
 */
inline auto CPU::inc(uint8_t reg) -> void
{
    uint8_t value = _registers.regs8[reg];

    _registers.regs8[reg]++;
    _registers.regs8[Reg8::F] &= Flags::c;
    if (_registers.regs8[reg] == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if ((value & 0xF) == 0xF)
        _registers.regs8[Reg8::F] |= Flags::h;
}

/**
 * @brief Increments the value at the memory address pointed to by HL and updates flags.
 */
inline auto CPU::inc_hl() -> void
{
    uint16_t addr = _registers.regs16[Reg16::HL];
    uint8_t old_value = _ram[addr];
    uint8_t new_value = old_value + 1;

    write_ram(addr, new_value);
    _registers.regs8[Reg8::F] &= Flags::c;
    if (new_value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if ((old_value & 0x0F) == 0x0F)
        _registers.regs8[Reg8::F] |= Flags::h;
}

/**
 * @brief Decrements the specified 8-bit register and updates flags.
 * @param reg The index of the 8-bit register to decrement.
 */
inline auto CPU::dec(uint8_t reg) -> void
{
    _registers.regs8[reg]--;
    _registers.regs8[Reg8::F] &= Flags::c;
    _registers.regs8[Reg8::F] |= Flags::n;
    if (_registers.regs8[reg] == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if ((_registers.regs8[reg] & 0xF) == 0xF)
        _registers.regs8[Reg8::F] |= Flags::h;
}

/**
 * @brief Decrements the value at the memory address pointed to by HL and updates flags.
 */
inline auto CPU::dec_hl() -> void
{
    uint8_t a = _ram[_registers.regs16[Reg16::HL]];

    a--;
    write_ram(_registers.regs16[Reg16::HL], a);
    _registers.regs8[Reg8::F] &= Flags::c;
    _registers.regs8[Reg8::F] |= Flags::n;
    if (a == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if ((a & 0xF) == 0xF)
        _registers.regs8[Reg8::F] |= Flags::h;
}

/**
 * @brief Performs a bitwise AND between the accumulator and the specified value, updating flags.
 * @param value The value to AND with the accumulator.
 */
inline auto CPU::and_(uint8_t value) -> void
{
    _registers.regs8[Reg8::A] &= value;
    _registers.regs8[Reg8::F] = Flags::h;
    if (_registers.regs8[Reg8::A] == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
}

/**
 * @brief Performs a bitwise OR between the accumulator and the specified value, updating flags.
 * @param value The value to OR with the accumulator.
 */
inline auto CPU::or_(uint8_t value) -> void
{
    _registers.regs8[Reg8::A] |= value;
    _registers.regs8[Reg8::F] = 0;
    if (_registers.regs8[Reg8::A] == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
}

/**
 * @brief Performs a bitwise XOR between the accumulator and the specified value, updating flags.
 * @param value The value to XOR with the accumulator.
 */
inline auto CPU::xor_(uint8_t value) -> void
{
    _registers.regs8[Reg8::A] ^= value;
    _registers.regs8[Reg8::F] = 0;
    if (_registers.regs8[Reg8::A] == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
}

/**
 * @brief Complements the carry flag.
 */
inline auto CPU::ccf() -> void
{
    uint8_t flags = _registers.regs8[Reg8::F];
    _registers.regs8[Reg8::F] = (~flags & 0x10) | (flags & 0x80);
}

/**
 * @brief Sets the carry flag to 1.
 */
inline auto CPU::scf() -> void
{
    uint8_t flags = _registers.regs8[Reg8::F];
    _registers.regs8[Reg8::F] = 0x10 | (flags & 0x80);
}

/**
 * @brief Adjusts the accumulator to binary-coded decimal (BCD) after an arithmetic operation.
 */
inline auto CPU::daa() -> void
{
    uint16_t a = _registers.regs8[Reg8::A] | ((_registers.regs8[Reg8::F] & 0x7F) << 4);
    uint16_t result = intToBcd[a];

    _registers.regs8[Reg8::A] = result & 0xFF;
    _registers.regs8[Reg8::F] &= Flags::n;
    if ((result & 0xFF) == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    if (result & 0x200)
        _registers.regs8[Reg8::F] |= Flags::c;
}

/**
 * @brief Complements the accumulator (inverts all bits).
 */
inline auto CPU::cpl() -> void
{
    _registers.regs8[Reg8::A] = ~_registers.regs8[Reg8::A];
    _registers.regs8[Reg8::F] |= Flags::n;
    _registers.regs8[Reg8::F] |= Flags::h;
}

/**
 * @brief Increments the specified 16-bit register.
 * @param reg The index of the 16-bit register to increment.
 */
inline auto CPU::inc16(uint8_t reg) -> void
{
    _registers.regs16[reg]++;
}

/**
 * @brief Decrements the specified 16-bit register.
 * @param reg The index of the 16-bit register to decrement.
 */
inline auto CPU::dec16(uint8_t reg) -> void
{
    _registers.regs16[reg]--;
}

/**
 * @brief Adds a 16-bit value to HL and updates flags.
 * @param rr The 16-bit value to add to HL.
 */
inline auto CPU::add_hl(uint16_t rr) -> void
{
    uint32_t hl = _registers.regs16[Reg16::HL];
    uint32_t result = hl + rr;
    uint16_t half_result = (hl & 0xFFF) + (rr & 0xFFF);

    _registers.regs16[Reg16::HL] = result & 0xFFFF;
    _registers.regs8[Reg8::F] &= Flags::z;
    if (half_result > 0xFFF)
        _registers.regs8[Reg8::F] |= Flags::h;
    if (result > 0xFFFF)
        _registers.regs8[Reg8::F] |= Flags::c;
}

/**
 * @brief Adds a signed 8-bit value to the stack pointer and updates flags.
 * @param value The signed 8-bit value to add to the stack pointer.
 */
inline auto CPU::add_sp(int8_t value) -> void
{
    uint16_t sp = _registers.sp;
    uint16_t result = sp + value;
    uint16_t half_result = (sp & 0xF) + (value & 0xF);

    _registers.sp = result & 0xFFFF;
    _registers.regs8[Reg8::F] = 0;
    if (half_result > 0xF)
        _registers.regs8[Reg8::F] |= Flags::h;
    if ((sp & 0xFF) + static_cast<uint8_t>(value) > 0xFF)
        _registers.regs8[Reg8::F] |= Flags::c;
}

/**
 * @brief Rotates the bits of the value left, updating the carry flag and optionally the zero flag.
 * @param value The value to rotate.
 * @param zflag Indicates whether the zero flag should be updated if the result is zero.
 * @return The value after rotation.
 */
inline auto CPU::rotl(uint8_t value, bool zflag) -> uint8_t
{
    value = (value << 1) | (value >> 7);
    _registers.regs8[Reg8::F] = (value & 1) << 4; // C flag
    if (zflag && value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Rotates the bits of the value left through the carry flag, updating flags.
 * @param value The value to rotate.
 * @param zflag Indicates whether the zero flag should be updated if the result is zero.
 * @return The value after rotation.
 */
inline auto CPU::rotlc(uint8_t value, bool zflag) -> uint8_t
{
    int8_t carry = (_registers.regs8[Reg8::F] >> 4) & 1;

    _registers.regs8[Reg8::F] = (value >> 3) & 0x10; // C flag
    value = (value << 1) | carry;
    if (zflag && value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Rotates the bits of the value right, updating the carry flag and optionally the zero flag.
 * @param value The value to rotate.
 * @param zflag Indicates whether the zero flag should be updated if the result is zero.
 * @return The value after rotation.
 */
inline auto CPU::rotr(uint8_t value, bool zflag) -> uint8_t
{
    value = (value >> 1) | (value << 7);
    _registers.regs8[Reg8::F] = (value >> 3) & 0x10; // C flag
    if (zflag && value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Rotates the bits of the value right through the carry flag, updating flags.
 * @param value The value to rotate.
 * @param zflag Indicates whether the zero flag should be updated if the result is zero.
 * @return The value after rotation.
 */
inline auto CPU::rotrc(uint8_t value, bool zflag) -> uint8_t
{
    int8_t carry = (_registers.regs8[Reg8::F] >> 4) & 1;

    _registers.regs8[Reg8::F] = (value & 1) << 4; // C flag
    value = (value >> 1) | (carry << 7);
    if (zflag && value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Shifts the bits of the value left, updating the carry and zero flags.
 * @param value The value to shift.
 * @return The value after shifting.
 */
inline auto CPU::shiftl(uint8_t value) -> uint8_t
{
    _registers.regs8[Reg8::F] = (value >> 3) & 0x10; // C flag
    value = (value << 1) & 0xFE;
    if (value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Shifts the bits of the value right logically, updating the carry and zero flags.
 * @param value The value to shift.
 * @return The value after shifting.
 */
inline auto CPU::shiftr(uint8_t value) -> uint8_t
{
    _registers.regs8[Reg8::F] = (value << 4) & 0x10; // C flag
    value >>= 1;
    if (value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Shifts the bits of the value right arithmetically, updating the carry and zero flags.
 * @param value The value to shift.
 * @return The value after shifting.
 */
inline auto CPU::shiftr2(uint8_t value) -> uint8_t
{
    _registers.regs8[Reg8::F] = (value << 4) & 0x10; // C flag
    value = (value & 0x80) | (value >> 1);
    if (value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Swaps the high and low nibbles of the value and updates the zero flag.
 * @param value The value whose nibbles are to be swapped.
 * @return The value with swapped nibbles.
 */
inline auto CPU::swap(uint8_t value) -> uint8_t
{
    value = (value >> 4) | (value << 4);
    _registers.regs8[Reg8::F] = 0;
    if (value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    return (value);
}

/**
 * @brief Tests a specific bit in a value and updates flags.
 * @param bit The bit position to test (0-7).
 * @param value The value in which to test the bit.
 */
inline auto CPU::bit_test(uint8_t bit, uint8_t value) -> void
{
    value &= (1 << bit);
    _registers.regs8[Reg8::F] &= Flags::c;
    if (value == 0)
        _registers.regs8[Reg8::F] |= Flags::z;
    _registers.regs8[Reg8::F] |= Flags::h;
}

/**
 * @brief Sets a specific bit in a value to 1.
 * @param bit The bit position to set (0-7).
 * @param value The value in which to set the bit.
 * @return The value with the specified bit set to 1.
 */
auto CPU::bit_set(uint8_t bit, uint8_t value) -> uint8_t
{
    return (value | (1 << bit));
}

/**
 * @brief Resets a specific bit in a value to 0.
 * @param bit The bit position to reset (0-7).
 * @param value The value in which to reset the bit.
 * @return The value with the specified bit reset to 0.
 */
auto CPU::bit_reset(uint8_t bit, uint8_t value) -> uint8_t
{
    value &= ~(1 << bit);
    return (value);
}

/**
 * @brief Jumps to the specified address by updating the program counter.
 * @param addr The address to jump to.
 */
inline auto CPU::jump(uint16_t addr) -> void
{
    _registers.pc = addr;
}

/**
 * @brief Performs a conditional jump based on the opcode and current flags.
 * @param opcode The opcode determining the condition.
 * @param addr The address to jump to if the condition is met.
 */
inline auto CPU::jump_conditionnal(uint8_t opcode, uint16_t addr) -> bool
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint8_t c = (_registers.regs8[Reg8::F] & Flags::c) >> 4;
    uint8_t z = (_registers.regs8[Reg8::F] & Flags::z) >> 7;

    if ((cc == 0 && z == 0) || (cc == 1 && z == 1) || (cc == 2 && c == 0) || (cc == 3 && c == 1))
    {
        _registers.pc = addr;
        return (true);
    }
    return (false);
}

/**
 * @brief Calls a subroutine at the specified address, pushing the current PC onto the stack.
 * @param addr The address of the subroutine to call.
 */
inline auto CPU::call(uint16_t addr) -> void
{
    _registers.sp--;
    _ram.write(_registers.sp--, (_registers.pc >> 8) & 0xFF);
    _ram.write(_registers.sp, _registers.pc & 0xFF);
    jump(addr);
}

/**
 * @brief Performs a conditional call based on the opcode and current flags.
 * @param opcode The opcode determining the condition.
 * @param addr The address to call if the condition is met.
 */
inline auto CPU::call_conditionnal(uint8_t opcode, uint16_t addr) -> void
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint8_t c = (_registers.regs8[Reg8::F] & Flags::c) >> 4;
    uint8_t z = (_registers.regs8[Reg8::F] & Flags::z) >> 7;

    if ((cc == 0 && z == 0) || (cc == 1 && z == 1) || (cc == 2 && c == 0) || (cc == 3 && c == 1))
        call(addr);
}

/**
 * @brief Returns from a subroutine by popping the PC from the stack.
 */
inline auto CPU::ret() -> void
{
    uint16_t addr = _ram[_registers.sp++];

    addr |= _ram[_registers.sp++] << 8;
    _registers.pc = addr;
}

/**
 * @brief Returns from a subroutine by popping the PC from the stack.
 */
inline auto CPU::reti() -> void
{
    uint16_t addr = _ram[_registers.sp++];

    addr |= _ram[_registers.sp++] << 8;
    _registers.pc = addr;
    _registers.ime = 1;
    _active_interruption = false;
}
/**
 * @brief Performs a conditional return based on the opcode and current flags.
 * @param opcode The opcode determining the condition.
 * @return `true` if the return was performed, `false` otherwise.
 */
inline auto CPU::ret_conditionnal(uint8_t opcode) -> bool
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint8_t c = (_registers.regs8[Reg8::F] & Flags::c) >> 4;
    uint8_t z = (_registers.regs8[Reg8::F] & Flags::z) >> 7;

    if ((cc == 0 && z == 0) || (cc == 1 && z == 1) || (cc == 2 && c == 0) || (cc == 3 && c == 1))
    {
        ret();
        return (true);
    }
    return (false);
}

/**
 * @brief Puts the CPU into a halted state.
 */
inline auto CPU::halt() -> void
{
    _registers.halt = 1;
}

/**
 * @brief Stops the CPU and resets the divider register.
 */
inline auto CPU::stop() -> void
{
    _registers.pc++;
    _ram.write_register(Timer::Register::DIV, 0);
    _ram.write_register(CPU::Register::IE, 0);
}

/**
 * @brief Disables interrupts.
 */
inline auto CPU::di() -> void
{
    _registers.ime = 0;
}

/**
 * @brief Enables interrupts.
 */
inline auto CPU::ei() -> void
{
    _registers.ime = 1;
    _active_interruption = true;
}

/**
 * @brief Performs a no-operation (NOP).
 */
inline auto CPU::nop() -> void
{
}

/**
 * @brief Handles an active interrupt by resetting the interrupt flag, disabling interrupts, and calling the handler.
 * @param bit Interrupt bit
 * @param addr The address of the interrupt handler.
 * @return The number of cycles taken (5).
 */
inline auto CPU::active_interrupt(uint8_t bit, uint16_t addr) -> uint8_t
{
    _ram.write_register(Register::IF, _ram[Register::IF] & ~bit);
    di();
    call(addr);
    return (5);
}
