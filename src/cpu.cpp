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

CPU::CPU(RamBus &ram) : mRAM(ram)
{
    mRegister.ie = 0;
    mRegister.pc = 0x100;
    mRegister.sp = 0xfffe;
    mMapReg = {Reg8::B, Reg8::C, Reg8::D, Reg8::E, Reg8::H, Reg8::L, Reg8::F, Reg8::A};
    mRegister.regs8[Reg8::A] = 0x01;
    mRegister.regs8[Reg8::F] = 0x00;
    mRegister.regs8[Reg8::B] = 0x00;
    mRegister.regs8[Reg8::C] = 0x13;
    mRegister.regs8[Reg8::D] = 0;
    mRegister.regs8[Reg8::E] = 0xd8;
    mRegister.regs8[Reg8::H] = 0x01;
    mRegister.regs8[Reg8::L] = 0x4d;
}

void CPU::debug(uint32_t cycles)
{
    // #ifdef A
    FILE *f = /*stdout; */ fopen("log", "a+");
    fprintf(f, "A:%2X F:", mRegister.regs8[Reg8::A]);
    if (mRegister.regs8[Reg8::F] & 0x80)
        fprintf(f, "Z");
    else
        fprintf(f, "-");
    if (mRegister.regs8[Reg8::F] & 0x40)
        fprintf(f, "N");
    else
        fprintf(f, "-");
    if (mRegister.regs8[Reg8::F] & 0x20)
        fprintf(f, "H");
    else
        fprintf(f, "-");
    if (mRegister.regs8[Reg8::F] & 0x10)
        fprintf(f, "C");
    else
        fprintf(f, "-");
    fprintf(f, " BC:%04X DE:%04x HL:%04x SP:%04x PC:%04x  IF:%02X (cy: %d)\n", mRegister.regs16[Reg16::BC],
            mRegister.regs16[Reg16::DE], mRegister.regs16[Reg16::HL], mRegister.sp, mRegister.pc, mRAM[IF], cycles);
    fclose(f);
    // #endif
}

uint8_t CPU::step()
{
    int8_t cycles_count = 0;

    if (mRegister.ie == 1)
    {
        uint8_t activeInterrupt = mRAM[Register::IE] & mRAM[Register::IF];

        if (activeInterrupt & 0x1)
            cycles_count = active_interrupt(0x40);
        else if (activeInterrupt & 0x2)
            cycles_count = active_interrupt(0x48);
        else if (activeInterrupt & 0x4)
            cycles_count = active_interrupt(0x50);
        else if (activeInterrupt & 0x10)
            cycles_count = active_interrupt(0x58);
        else if (activeInterrupt & 0x20)
            cycles_count = active_interrupt(0x60);
    }
    cycles_count += decode();
    return (cycles_count);
}

void CPU::save_state(const std::string &rom_file)
{
    std::string filename = rom_file + ".state";
    std::ofstream output(filename.data(), std::ios::binary);
    std::streamsize bytesRead;

    if (output)
    {
        output.write(reinterpret_cast<const char *>(&mRegister), sizeof(Registers));
        output.write(reinterpret_cast<const char *>(mRAM.data()), Ram::ram_size);
    }
}

void CPU::load_state(const std::string &rom_file)
{
    std::string filename = rom_file + ".state";
    std::ifstream input(filename.data(), std::ios::binary);
    std::streamsize bytesRead;
    std::array<unsigned char, Ram::ram_size> block;

    if (input)
    {
        input.read(reinterpret_cast<char *>(&mRegister), sizeof(Registers));
        input.read(reinterpret_cast<char *>(block.data()), Ram::ram_size);
        bytesRead = input.gcount();
        if (bytesRead == Ram::ram_size)
            mRAM.write_range(block.data(), Ram::ram_size, 0);
    }
}

uint8_t CPU::decode()
{
    uint8_t opcode = mRAM[mRegister.pc++];
    uint8_t reg = mMapReg[(opcode >> 3) & 0x7];
    uint16_t address;
    uint16_t value;
    uint16_t carry = (mRegister.regs8[Reg8::F] >> 4) & 1;
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
        load_register(reg, mRegister.regs8[mMapReg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0x06):
    case (0x0e):
    case (0x16):
    case (0x1e):
    case (0x26):
    case (0x2e):
    case (0x3e):
        load_register(reg, mRAM[mRegister.pc++]);
        cycles_count = 2;
        break;
    case (0x46):
    case (0x4e):
    case (0x56):
    case (0x5e):
    case (0x66):
    case (0x6e):
    case (0x7e):
        load_register(reg, mRAM[mRegister.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0x70):
    case (0x71):
    case (0x72):
    case (0x73):
    case (0x74):
    case (0x75):
    case (0x77):
        write_ram(mRegister.regs16[Reg16::HL], mRegister.regs8[mMapReg[opcode & 0x7]]);
        cycles_count = 2;
        break;
    case (0x36):
        write_ram(mRegister.regs16[Reg16::HL], mRAM[mRegister.pc++]);
        cycles_count = 3;
        break;
    case (0x0a):
        load_register(Reg8::A, mRAM[mRegister.regs16[Reg16::BC]]);
        cycles_count = 2;
        break;
    case (0x1a):
        load_register(Reg8::A, mRAM[mRegister.regs16[Reg16::DE]]);
        cycles_count = 2;
        break;
    case (0xfa):
        value = mRAM[mRegister.pc++];
        value |= mRAM[mRegister.pc++] << 8;
        load_register(Reg8::A, mRAM[value]);
        cycles_count = 4;
        break;
    case (0x3a):
        load_register(Reg8::A, mRAM[mRegister.regs16[Reg16::HL]]);
        mRegister.regs16[Reg16::HL]--;
        cycles_count = 2;
        break;
    case (0x2a):
        load_register(Reg8::A, mRAM[mRegister.regs16[Reg16::HL]]);
        mRegister.regs16[Reg16::HL]++;
        cycles_count = 2;
        break;
    case (0xf2):
        address = 0xFF00 | mRegister.regs8[Reg8::C];
        load_register(Reg8::A, mRAM[address]);
        cycles_count = 2;
        break;
    case (0xf0):
        address = 0xFF00 | mRAM[mRegister.pc++];
        load_register(Reg8::A, mRAM[address]);
        cycles_count = 3;
        break;
    case (0x02):
        write_ram(mRegister.regs16[Reg16::BC], mRegister.regs8[Reg8::A]);
        cycles_count = 2;
        break;
    case (0x12):
        write_ram(mRegister.regs16[Reg16::DE], mRegister.regs8[Reg8::A]);
        cycles_count = 2;
        break;
    case (0xea):
        value = mRAM[mRegister.pc++];
        value |= mRAM[mRegister.pc++] << 8;
        write_ram(value, mRegister.regs8[Reg8::A]);
        cycles_count = 4;
        break;
    case (0xe2):
        address = 0xFF00 | mRegister.regs8[Reg8::C];
        write_ram(address, mRegister.regs8[Reg8::A]);
        cycles_count = 2;
        break;
    case (0xe0):
        address = 0xFF00 | mRAM[mRegister.pc++];
        write_ram(address, mRegister.regs8[Reg8::A]);
        cycles_count = 3;
        break;
    case (0x32):
        write_ram(mRegister.regs16[Reg16::HL], mRegister.regs8[Reg8::A]);
        mRegister.regs16[Reg16::HL]--;
        cycles_count = 2;
        break;
    case (0x22):
        write_ram(mRegister.regs16[Reg16::HL], mRegister.regs8[Reg8::A]);
        mRegister.regs16[Reg16::HL]++;
        cycles_count = 2;
        break;
    case (0x01):
    case (0x11):
    case (0x21):
        reg = (opcode >> 4) & 0x3;
        value = mRAM[mRegister.pc++];
        value |= (mRAM[mRegister.pc++] << 8);
        load_register16(reg, value);
        cycles_count = 3;
        break;
    case (0x31):
        value = mRAM[mRegister.pc++];
        value |= (mRAM[mRegister.pc++] << 8);
        mRegister.sp = value;
        cycles_count = 3;
        break;
    case (0x08):
        address = mRAM[mRegister.pc++];
        address |= (mRAM[mRegister.pc++] << 8);
        write_ram16(address, mRegister.sp);
        cycles_count = 5;
        break;
    case (0xf9):
        mRegister.sp = mRegister.regs16[Reg16::HL];
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
        add_stack(mRAM[mRegister.pc++]);
        cycles_count = 3;
        break;
    case (0x80):
    case (0x81):
    case (0x82):
    case (0x83):
    case (0x84):
    case (0x85):
    case (0x87):
        add(mRegister.regs8[mMapReg[opcode & 0x7]], 0);
        cycles_count = 1;
        break;
    case (0x86):
        add(mRAM[mRegister.regs16[Reg16::HL]], 0);
        cycles_count = 2;
        break;
    case (0xc6):
        add(mRAM[mRegister.pc++], 0);
        cycles_count = 2;
        break;
    case (0x88):
    case (0x89):
    case (0x8a):
    case (0x8b):
    case (0x8c):
    case (0x8d):
    case (0x8f):
        add(mRegister.regs8[mMapReg[opcode & 0x7]], carry);
        cycles_count = 1;
        break;
    case (0x8e):
        add(mRAM[mRegister.regs16[Reg16::HL]], carry);
        cycles_count = 2;
        break;
    case (0xce):
        add(mRAM[mRegister.pc++], carry);
        cycles_count = 2;
        break;
    case (0x90):
    case (0x91):
    case (0x92):
    case (0x93):
    case (0x94):
    case (0x95):
    case (0x97):
        sub(mRegister.regs8[mMapReg[opcode & 0x7]], 0);
        cycles_count = 1;
        break;
    case (0x96):
        sub(mRAM[mRegister.regs16[Reg16::HL]], 0);
        cycles_count = 2;
        break;
    case (0xd6):
        sub(mRAM[mRegister.pc++], 0);
        cycles_count = 2;
        break;
    case (0x98):
    case (0x99):
    case (0x9a):
    case (0x9b):
    case (0x9c):
    case (0x9d):
    case (0x9f):
        sub(mRegister.regs8[mMapReg[opcode & 0x7]], carry);
        cycles_count = 1;
        break;
    case (0x9e):
        sub(mRAM[mRegister.regs16[Reg16::HL]], carry);
        cycles_count = 2;
        break;
    case (0xde):
        sub(mRAM[mRegister.pc++], carry);
        cycles_count = 2;
        break;
    case (0xb8):
    case (0xb9):
    case (0xba):
    case (0xbb):
    case (0xbc):
    case (0xbd):
    case (0xbf):
        cp(mRegister.regs8[mMapReg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xbe):
        cp(mRAM[mRegister.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xfe):
        cp(mRAM[mRegister.pc++]);
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
        and_(mRegister.regs8[mMapReg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xa6):
        and_(mRAM[mRegister.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xe6):
        and_(mRAM[mRegister.pc++]);
        cycles_count = 2;
        break;
    case (0xb0):
    case (0xb1):
    case (0xb2):
    case (0xb3):
    case (0xb4):
    case (0xb5):
    case (0xb7):
        or_(mRegister.regs8[mMapReg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xb6):
        or_(mRAM[mRegister.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xf6):
        or_(mRAM[mRegister.pc++]);
        cycles_count = 2;
        break;
    case (0xa8):
    case (0xa9):
    case (0xaa):
    case (0xab):
    case (0xac):
    case (0xad):
    case (0xaf):
        xor_(mRegister.regs8[mMapReg[opcode & 0x7]]);
        cycles_count = 1;
        break;
    case (0xae):
        xor_(mRAM[mRegister.regs16[Reg16::HL]]);
        cycles_count = 2;
        break;
    case (0xee):
        xor_(mRAM[mRegister.pc++]);
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
        mRegister.sp++;
        cycles_count = 2;
        break;
    case (0x0b):
    case (0x1b):
    case (0x2b):
        dec16((opcode >> 4) & 0x3);
        cycles_count = 2;
        break;
    case (0x3b):
        mRegister.sp--;
        cycles_count = 2;
        break;
    case (0x09):
    case (0x19):
    case (0x29):
        add_hl(mRegister.regs16[(opcode >> 4) & 0x3]);
        cycles_count = 2;
        break;
    case (0x39):
        add_hl(mRegister.sp);
        cycles_count = 2;
        break;
    case (0xe8):
        add_sp(mRAM[mRegister.pc++]);
        cycles_count = 4;
        break;
    case (0x07):
        mRegister.regs8[Reg8::A] = rotl(mRegister.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0x0f):
        mRegister.regs8[Reg8::A] = rotr(mRegister.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0x17):
        mRegister.regs8[Reg8::A] = rotlc(mRegister.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0x1f):
        mRegister.regs8[Reg8::A] = rotrc(mRegister.regs8[Reg8::A], false);
        cycles_count = 1;
        break;
    case (0xc3):
        value = mRAM[mRegister.pc++];
        value |= (mRAM[mRegister.pc++] << 8);
        jump(value);
        cycles_count = 4;
        break;
    case (0xe9):
        jump(mRegister.regs16[Reg16::HL]);
        cycles_count = 1;
        break;
    case (0xc2):
    case (0xca):
    case (0xd2):
    case (0xda):
        value = mRAM[mRegister.pc++];
        value |= (mRAM[mRegister.pc++] << 8);
        jump_conditionnal(opcode, value);
        cycles_count = 4;
        if (mRegister.pc != value)
            cycles_count--;
        break;
    case (0x18):
        relative = mRAM[mRegister.pc++];
        jump(mRegister.pc + relative);
        cycles_count = 3;
        break;
    case (0x20):
    case (0x28):
    case (0x30):
    case (0x38):
        relative = mRAM[mRegister.pc++];
        value = mRegister.pc + relative;
        jump_conditionnal(opcode, value);
        cycles_count = 3;
        if (mRegister.pc != value)
            cycles_count--;
        break;
    case (0xcd):
        value = mRAM[mRegister.pc++];
        value |= (mRAM[mRegister.pc++] << 8);
        call(value);
        cycles_count = 6;
        break;
    case (0xc4):
    case (0xcc):
    case (0xd4):
    case (0xdc):
        value = mRAM[mRegister.pc++];
        value |= (mRAM[mRegister.pc++] << 8);
        call_conditionnal(opcode, value);
        cycles_count = 6;
        if (mRegister.pc != value)
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
        ei();
        ret();
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
        HALT();
        cycles_count = 1;
        break;
    case (0x10):
        STOP();
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

inline uint8_t CPU::decodeExtendOpcode()
{
    uint8_t opcode = mRAM[mRegister.pc++];
    uint16_t address;
    uint8_t bit;
    uint8_t reg = mMapReg[opcode & 0x7];
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
        mRegister.regs8[reg] = rotl(mRegister.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x06):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = rotr(mRegister.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x0e):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = rotlc(mRegister.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x16):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = rotrc(mRegister.regs8[reg], true);
        cycles_count = 2;
        break;
    case (0x1e):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = shiftl(mRegister.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x26):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = shiftr2(mRegister.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x2e):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = swap(mRegister.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x36):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = shiftr(mRegister.regs8[reg]);
        cycles_count = 2;
        break;
    case (0x3e):
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        bit_test(bit, mRegister.regs8[reg]);
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
        value = mRAM[mRegister.regs16[Reg16::HL]];
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
        mRegister.regs8[reg] = bit_reset(bit, mRegister.regs8[reg]);
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
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
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
        mRegister.regs8[reg] = bit_set(bit, mRegister.regs8[reg]);
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
        address = mRegister.regs16[Reg16::HL];
        value = mRAM[address];
        bit = (opcode >> 3) & 0x7;
        write_ram(address, bit_set(bit, value));
        cycles_count = 4;
        break;
    }
    return (cycles_count);
}

inline void CPU::load_register(uint8_t reg, uint8_t value)
{
    mRegister.regs8[reg] = value;
}

inline void CPU::write_ram(uint16_t address, uint8_t value)
{
    mRAM.write(address, value);
}

inline void CPU::load_register16(uint8_t reg, uint16_t value)
{
    mRegister.regs16[reg] = value;
}

inline void CPU::write_ram16(uint16_t address, uint16_t value)
{
    mRAM.write(address++, value & 0xFF);
    mRAM.write(address, (value >> 8) & 0xFF);
}

inline void CPU::push(uint8_t reg)
{
    mRegister.sp--;
    mRAM.write(mRegister.sp--, (mRegister.regs16[reg] >> 8) & 0xFF);
    mRAM.write(mRegister.sp, mRegister.regs16[reg] & 0xFF);
}

inline void CPU::pop(uint8_t reg)
{
    uint16_t Value = mRAM[mRegister.sp++];

    Value |= (mRAM[mRegister.sp++] << 8);
    mRegister.regs16[reg] = Value;
    mRegister.regs8[Reg8::F] &= 0xF0;
}

inline void CPU::add_stack(int8_t value)
{
    uint16_t sp = mRegister.sp;
    uint16_t result = sp + value;
    uint16_t half_result = (sp & 0xF) + (value & 0xF);

    mRegister.regs16[Reg16::HL] = result & 0xFFFF;
    mRegister.regs8[Reg8::F] = 0;
    if (half_result > 0xF)
        mRegister.regs8[Reg8::F] |= Flags::h;
    if ((sp & 0xFF) + static_cast<uint8_t>(value) > 0xFF)
        mRegister.regs8[Reg8::F] |= Flags::c;
}

inline void CPU::add(uint8_t value, uint8_t carry)
{
    uint16_t a = mRegister.regs8[Reg8::A];
    uint16_t result = a + value + carry;
    uint8_t half_result = (a & 0x0F) + (value & 0x0F) + (carry & 0x0F);

    mRegister.regs8[Reg8::A] = result & 0xFF;
    mRegister.regs8[Reg8::F] = 0;
    if (mRegister.regs8[Reg8::A] == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if (half_result > 0x0F)
        mRegister.regs8[Reg8::F] |= Flags::h;
    if (result > 0xFF)
        mRegister.regs8[Reg8::F] |= Flags::c;
}

inline void CPU::sub(uint8_t value, uint8_t carry)
{
    uint8_t a = mRegister.regs8[Reg8::A];

    mRegister.regs8[Reg8::A] = a - value - carry;
    mRegister.regs8[Reg8::F] = Flags::n;
    if (mRegister.regs8[Reg8::A] == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if ((a & 0x0F) < ((value & 0x0F) + (carry & 0x0F)))
        mRegister.regs8[Reg8::F] |= Flags::h;
    if (a < (value + carry))
        mRegister.regs8[Reg8::F] |= Flags::c;
}

inline void CPU::cp(uint8_t value)
{
    uint8_t a = mRegister.regs8[Reg8::A];
    uint8_t result = a - value;

    mRegister.regs8[Reg8::F] = Flags::n;
    if (result == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if ((a & 0x0F) < (value & 0x0F))
        mRegister.regs8[Reg8::F] |= Flags::h;
    if (a < value)
        mRegister.regs8[Reg8::F] |= Flags::c;
}

inline void CPU::inc(uint8_t reg)
{
    uint8_t value = mRegister.regs8[reg];

    mRegister.regs8[reg]++;
    mRegister.regs8[Reg8::F] &= Flags::c;
    if (mRegister.regs8[reg] == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if ((value & 0xF) == 0xF)
        mRegister.regs8[Reg8::F] |= Flags::h;
}

inline void CPU::inc_hl()
{
    uint16_t addr = mRegister.regs16[Reg16::HL];
    uint8_t old_value = mRAM[addr];
    uint8_t new_value = old_value + 1;

    write_ram(addr, new_value);
    mRegister.regs8[Reg8::F] &= Flags::c;
    if (new_value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if ((old_value & 0x0F) == 0x0F)
        mRegister.regs8[Reg8::F] |= Flags::h;
}

inline void CPU::dec(uint8_t reg)
{
    mRegister.regs8[reg]--;
    mRegister.regs8[Reg8::F] &= Flags::c;
    mRegister.regs8[Reg8::F] |= Flags::n;
    if (mRegister.regs8[reg] == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if ((mRegister.regs8[reg] & 0xF) == 0xF)
        mRegister.regs8[Reg8::F] |= Flags::h;
}

inline void CPU::dec_hl()
{
    uint8_t a = mRAM[mRegister.regs16[Reg16::HL]];

    a--;
    write_ram(mRegister.regs16[Reg16::HL], a);
    mRegister.regs8[Reg8::F] &= Flags::c;
    mRegister.regs8[Reg8::F] |= Flags::n;
    if (a == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if ((a & 0xF) == 0xF)
        mRegister.regs8[Reg8::F] |= Flags::h;
}

inline void CPU::and_(uint8_t value)
{
    mRegister.regs8[Reg8::A] &= value;
    mRegister.regs8[Reg8::F] = Flags::h;
    if (mRegister.regs8[Reg8::A] == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
}

inline void CPU::or_(uint8_t value)
{
    mRegister.regs8[Reg8::A] |= value;
    mRegister.regs8[Reg8::F] = 0;
    if (mRegister.regs8[Reg8::A] == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
}

inline void CPU::xor_(uint8_t value)
{
    mRegister.regs8[Reg8::A] ^= value;
    mRegister.regs8[Reg8::F] = 0;
    if (mRegister.regs8[Reg8::A] == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
}

inline void CPU::ccf()
{
    uint8_t flags = mRegister.regs8[Reg8::F];
    mRegister.regs8[Reg8::F] = (~flags & 0x10) | (flags & 0x80);
}

inline void CPU::scf()
{
    uint8_t flags = mRegister.regs8[Reg8::F];
    mRegister.regs8[Reg8::F] = 0x10 | (flags & 0x80);
}

inline void CPU::daa()
{
    uint16_t a = mRegister.regs8[Reg8::A] | ((mRegister.regs8[Reg8::F] & 0x7F) << 4);
    uint16_t result = intToBcd[a];

    mRegister.regs8[Reg8::A] = result & 0xFF;
    mRegister.regs8[Reg8::F] &= Flags::n;
    if ((result & 0xFF) == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    if (result & 0x200)
        mRegister.regs8[Reg8::F] |= Flags::c;
}

inline void CPU::cpl()
{
    mRegister.regs8[Reg8::A] = ~mRegister.regs8[Reg8::A];
    mRegister.regs8[Reg8::F] |= Flags::n;
    mRegister.regs8[Reg8::F] |= Flags::h;
}

inline void CPU::inc16(uint8_t reg)
{
    mRegister.regs16[reg]++;
}

inline void CPU::dec16(uint8_t reg)
{
    mRegister.regs16[reg]--;
}

inline void CPU::add_hl(uint16_t rr)
{
    uint32_t hl = mRegister.regs16[Reg16::HL];
    uint32_t result = hl + rr;
    uint16_t half_result = (hl & 0xFFF) + (rr & 0xFFF);

    mRegister.regs16[Reg16::HL] = result & 0xFFFF;
    mRegister.regs8[Reg8::F] &= Flags::z;
    if (half_result > 0xFFF)
        mRegister.regs8[Reg8::F] |= Flags::h;
    if (result > 0xFFFF)
        mRegister.regs8[Reg8::F] |= Flags::c;
}

inline void CPU::add_sp(int8_t value)
{
    uint16_t sp = mRegister.sp;
    uint16_t result = sp + value;
    uint16_t half_result = (sp & 0xF) + (value & 0xF);

    mRegister.sp = result & 0xFFFF;
    mRegister.regs8[Reg8::F] = 0;
    if (half_result > 0xF)
        mRegister.regs8[Reg8::F] |= Flags::h;
    if ((sp & 0xFF) + static_cast<uint8_t>(value) > 0xFF)
        mRegister.regs8[Reg8::F] |= Flags::c;
}

inline uint8_t CPU::rotl(uint8_t value, bool zflag)
{
    value = (value << 1) | (value >> 7);
    mRegister.regs8[Reg8::F] = (value & 1) << 4; // C flag
    if (zflag && value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline uint8_t CPU::rotlc(uint8_t value, bool zflag)
{
    int8_t carry = (mRegister.regs8[Reg8::F] >> 4) & 1;

    mRegister.regs8[Reg8::F] = (value >> 3) & 0x10; // C flag
    value = (value << 1) | carry;
    if (zflag && value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline uint8_t CPU::rotr(uint8_t value, bool zflag)
{
    value = (value >> 1) | (value << 7);
    mRegister.regs8[Reg8::F] = (value >> 3) & 0x10; // C flag
    if (zflag && value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline uint8_t CPU::rotrc(uint8_t value, bool zflag)
{
    int8_t carry = (mRegister.regs8[Reg8::F] >> 4) & 1;

    mRegister.regs8[Reg8::F] = (value & 1) << 4; // C flag
    value = (value >> 1) | (carry << 7);
    if (zflag && value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline uint8_t CPU::shiftl(uint8_t value)
{
    mRegister.regs8[Reg8::F] = (value >> 3) & 0x10; // C flag
    value = (value << 1) & 0xFE;
    if (value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline uint8_t CPU::shiftr(uint8_t value)
{
    mRegister.regs8[Reg8::F] = (value << 4) & 0x10; // C flag
    value >>= 1;
    if (value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline uint8_t CPU::shiftr2(uint8_t value)
{
    mRegister.regs8[Reg8::F] = (value << 4) & 0x10; // C flag
    value = (value & 0x80) | (value >> 1);
    if (value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline uint8_t CPU::swap(uint8_t value)
{
    value = (value >> 4) | (value << 4);
    mRegister.regs8[Reg8::F] = 0;
    if (value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    return (value);
}

inline void CPU::bit_test(uint8_t bit, uint8_t value)
{
    value &= (1 << bit);
    mRegister.regs8[Reg8::F] &= Flags::c;
    if (value == 0)
        mRegister.regs8[Reg8::F] |= Flags::z;
    mRegister.regs8[Reg8::F] |= Flags::h;
}

uint8_t CPU::bit_set(uint8_t bit, uint8_t value)
{
    return (value | (1 << bit));
}

uint8_t CPU::bit_reset(uint8_t bit, uint8_t value)
{
    value &= ~(1 << bit);
    return (value);
}

inline void CPU::jump(uint16_t addr)
{
    mRegister.pc = addr;
}

inline void CPU::jump_conditionnal(uint8_t opcode, uint16_t addr)
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint8_t c = (mRegister.regs8[Reg8::F] & Flags::c) >> 4;
    uint8_t z = (mRegister.regs8[Reg8::F] & Flags::z) >> 7;

    if ((cc == 0 && z == 0) || (cc == 1 && z == 1) || (cc == 2 && c == 0) || (cc == 3 && c == 1))
        mRegister.pc = addr;
}

inline void CPU::call(uint16_t addr)
{
    mRegister.sp--;
    mRAM.write(mRegister.sp--, (mRegister.pc >> 8) & 0xFF);
    mRAM.write(mRegister.sp, mRegister.pc & 0xFF);
    jump(addr);
}

inline void CPU::call_conditionnal(uint8_t opcode, uint16_t addr)
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint8_t c = (mRegister.regs8[Reg8::F] & Flags::c) >> 4;
    uint8_t z = (mRegister.regs8[Reg8::F] & Flags::z) >> 7;

    if ((cc == 0 && z == 0) || (cc == 1 && z == 1) || (cc == 2 && c == 0) || (cc == 3 && c == 1))
        call(addr);
}

inline void CPU::ret()
{
    uint16_t addr = mRAM[mRegister.sp++];

    addr |= mRAM[mRegister.sp++] << 8;
    mRegister.pc = addr;
}

inline bool CPU::ret_conditionnal(uint8_t opcode)
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint8_t c = (mRegister.regs8[Reg8::F] & Flags::c) >> 4;
    uint8_t z = (mRegister.regs8[Reg8::F] & Flags::z) >> 7;

    if ((cc == 0 && z == 0) || (cc == 1 && z == 1) || (cc == 2 && c == 0) || (cc == 3 && c == 1))
    {
        ret();
        return (true);
    }
    return (false);
}

inline void CPU::HALT()
{
}

inline void CPU::STOP()
{
    mRAM.write(Timer::Register::DIV, 0);
}

inline void CPU::di()
{
    mRegister.ie = 0;
}

inline void CPU::ei()
{
    mRegister.ie = 1;
}

inline void CPU::nop()
{
}

inline uint8_t CPU::active_interrupt(uint16_t addr)
{
    mRAM.write(Register::IF, 0);
    di();
    call(addr);
    return (5);
}