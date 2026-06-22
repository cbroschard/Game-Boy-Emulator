// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <iostream>
#include "CPU.h"
#include "Bus.h"

CPU::CPU() :
    bus(nullptr),
    halted(false),
    stopped(false),
    IME(false),
    imeEnablePending(false)
{
    reset();
}

CPU::~CPU() = default;

void CPU::reset()
{
    // Reset registers
    AF                  = 0x0000;
    BC                  = 0x0000;
    DE                  = 0x0000;
    HL                  = 0x0000;
    SP                  = 0x0000;
    PC                  = 0x0000;

    halted              = false;
    stopped             = false;

    IME                 = false;
    imeEnablePending    = false;
}

int CPU::step()
{
    return 0;
}

uint8_t CPU::fetch8()
{
    if (!bus)
    {
        std::cerr << "CPU bus not attached\n";
        return 0xFF;
    }

    uint8_t value = bus->read(PC);
    PC++;
    return value;
}

uint16_t CPU::fetch16()
{
    uint8_t lo = fetch8();
    uint8_t hi = fetch8();

    return uint16_t(lo) | (uint16_t(hi) << 8);
}

int CPU::decodeExecute(uint8_t opcode)
{
    switch (opcode)
    {
        case 0x00: // NOP
        {
            return 4;
        }

        case 0x01: // LD BC,U16
        {
            setBC(fetch16());
            return 12;
        }

        case 0x02: // LD (BC),A
        {
            bus->write(getBC(), getA());
            return 8;
        }

        case 0x03: // INC BC
        {
            setBC(inc16(getBC()));
            return 8;
        }

        case 0x04: // INC B
        {
            setB(inc8(getB()));
            return 4;
        }

        case 0x05: // DEC B
        {
            setB(dec8(getB()));
            return 4;
        }

        case 0x06: // LD B, U8
        {
            setB(fetch8());
            return 8;
        }

        case 0x07: // RLCA
        {
            rlca();
            return 4;
        }

        case 0x08: // LD (U16),SP
        {
            uint16_t address = fetch16();
            uint16_t sp = getSP();

            bus->write(address, uint8_t(sp & 0x00FF));
            bus->write(address + 1, uint8_t(sp >> 8));

            return 20;
        }

        case 0x09: // ADD HL, BC
        {
            addHL(getBC());
            return 8;
        }

        case 0x0A: // LD A, (BC)
        {
            setA(bus->read(getBC()));
            return 8;
        }

        case 0x0B: // DEC BC
        {
            setBC(dec16(getBC()));
            return 8;
        }

        case 0x0C: // INC C
        {
            setC(inc8(getC()));
            return 4;
        }

        case 0x0D: // DEC C
        {
            setC(dec8(getC()));
            return 4;
        }

        case 0x0E: // LD C, U8
        {
            setC(fetch8());
            return 8;
        }

        case 0x0F: // RRCA
        {
            rrca();
            return 4;
        }

        case 0x10: // STOP
        {
            stopped = true;
            fetch8();
            return 4;
        }

        case 0x11: // LD DE, U16
        {
            setDE(fetch16());
            return 12;
        }

        case 0x12: // LD (DE), A
        {
            bus->write(getDE(), getA());
            return 8;
        }

        case 0x13: // INC DE
        {
            setDE(inc16(getDE()));
            return 8;
        }

        case 0x14: // INC D
        {
            setD(inc8(getD()));
            return 4;
        }

        case 0x15: // DEC D
        {
            setD(dec8(getD()));
            return 4;
        }

        case 0x16: // LD D, U8
        {
            setD(fetch8());
            return 8;
        }

        case 0x17: // RLA
        {
            rla();
            return 4;
        }

        case 0x18: // JR I8
        {
            int8_t offset = int8_t(fetch8());
            setPC(uint16_t(getPC() + offset));
            return 12;
        }

        case 0x19: // ADD HL, DE
        {
            addHL(getDE());
            return 8;
        }

        case 0x1A: // LD A, (DE)
        {
            setA(bus->read(getDE()));
            return 8;
        }

        case 0x1B: // DEC DE
        {
            setDE(dec16(getDE()));
            return 8;
        }

        case 0x1C: // INC E
        {
            setE(inc8(getE()));
            return 4;
        }

        case 0x1D: // DEC E
        {
            setE(dec8(getE()));
            return 4;
        }


        case 0x1E: // LD E, U8
        {
            setE(fetch8());
            return 8;
        }

        case 0x1F: // RRA
        {
            rra();
            return 4;
        }

        case 0x20: // JR NZ,i8
        {
            int8_t offset = int8_t(fetch8());

            if (!getFlag(Z))
            {
                setPC(uint16_t(getPC() + offset));
                return 12;
            }

            return 8;
        }

        case 0x21: // LD HL, U16
        {
            setHL(fetch16());
            return 12;
        }

        case 0x22: // LD (HL+),A
        {
            uint16_t address = getHL();

            bus->write(address, getA());
            setHL(uint16_t(address + 1));

            return 8;
        }

        case 0x23: // INC HL
        {
            setHL(inc16(getHL()));
            return 8;
        }

        case 0x24: // INC H
        {
            setH(inc8(getH()));
            return 4;
        }

        case 0x25: // DEC H
        {
            setH(dec8(getH()));
            return 4;
        }

        case 0x26: // LD H, U8
        {
            setH(fetch8());
            return 8;
        }

        case 0x27: // DAA
        {
            daa();
            return 4;
        }

        case 0x28: // JR Z,i8
        {
            int8_t offset = int8_t(fetch8());

            if (getFlag(Z))
            {
                setPC(uint16_t(getPC() + offset));
                return 12;
            }

            return 8;
        }

        case 0x29: // ADD HL, HL
        {
            addHL(getHL());
            return 8;
        }

        case 0x2A: // LD A,(HL+)
        {
            uint16_t address = getHL();

            setA(bus->read(address));
            setHL(uint16_t(address + 1));

            return 8;
        }

        case 0x2B: // DEC HL
        {
           setHL(dec16(getHL()));
           return 8;
        }

        case 0x2C: // INC L
        {
            setL(inc8(getL()));
            return 4;
        }

        case 0x2D: // DEC L
        {
            setL(dec8(getL()));
            return 4;
        }

        case 0x2E: // LD L, U8
        {
           setL(fetch8());
           return 8;
        }

        case 0x2F: // CPL
        {
            cpl();
            return 4;
        }

        case 0x30: // JR NC, I8
        {
            int8_t offset = int8_t(fetch8());

            if (!getFlag(C))
            {
                setPC(uint16_t(getPC() + offset));
                return 12;
            }

            return 8;
        }

        case 0x31: // LD SP, U16
        {
            setSP(fetch16());
            return 12;
        }

        case 0x32: // LD (HL-), A
        {
            uint16_t address = getHL();

            bus->write(address, getA());
            setHL(uint16_t(address - 1));

            return 8;
        }

        case 0x33: // INC SP
        {
            setSP(inc16(getSP()));
            return 8;
        }

        case 0x34: // INC (HL)
        {
            uint16_t address = getHL();
            uint8_t value = bus->read(address);
            uint8_t result = inc8(value);

            bus->write(address, result);

            return 12;
        }

        case 0x35: // DEC (HL)
        {
            uint16_t address = getHL();
            uint8_t value = bus->read(address);
            uint8_t result = dec8(value);

            bus->write(address, result);

            return 12;
        }

        case 0x36: // LD (HL), U8
        {
            uint16_t address = getHL();
            uint8_t value = fetch8();

            bus->write(address, value);

            return 12;
        }

        case 0x37: // SCF
        {
            scf();
            return 4;
        }

        case 0x38: // JR C, I8
        {
            int8_t offset = int8_t(fetch8());

            if (getFlag(C))
            {
                setPC(uint16_t(getPC() + offset));
                return 12;
            }

            return 8;
        }

        case 0x39: // ADD HL, SP
        {
            addHL(getSP());
            return 8;
        }

        case 0x3A: // LD A, (HL-)
        {
            uint16_t address = getHL();

            setA(bus->read(address));
            setHL(uint16_t(address - 1));

            return 8;
        }

        case 0x3B: // DEC SP
        {
            setSP(dec16(getSP()));
            return 8;
        }

        case 0x3C: // INC A
        {
            setA(inc8(getA()));
            return 4;
        }

        case 0x3D: // DEC A
        {
            setA(dec8(getA()));
            return 4;
        }

        case 0x3E: // LD A, U8
        {
            setA(fetch8());
            return 8;
        }

        case 0x3F: // CCF
        {
            ccf();
            return 4;
        }

        case 0x40: // LD B,B
        {
            setB(getB());
            return 4;
        }

        case 0x41: // LD B,C
        {
            setB(getC());
            return 4;
        }

        case 0x42: // LD B,D
        {
            setB(getD());
            return 4;
        }

        case 0x43: // LD B,E
        {
            setB(getE());
            return 4;
        }

        case 0x44: // LD B,H
        {
            setB(getH());
            return 4;
        }

        case 0x45: // LD B,L
        {
            setB(getL());
            return 4;
        }

        case 0x46: // LD B, (HL)
        {
            setB(bus->read(getHL()));
            return 8;
        }

        case 0x47: // LD B, A
        {
            setB(getA());
            return 4;
        }

        case 0x48: // LD C, B
        {
            setC(getB());
            return 4;
        }

        case 0x49: // LD C, C
        {
            setC(getC());
            return 4;
        }

        case 0x4A: // LD C, D
        {
            setC(getD());
            return 4;
        }

        case 0x4B: // LD C, E
        {
            setC(getE());
            return 4;
        }

        case 0x4C: // LD C, H
        {
            setC(getH());
            return 4;
        }

        case 0x4D: // LD C, L
        {
            setC(getL());
            return 4;
        }

        case 0x4E: // LD C, (HL)
        {
            setC(bus->read(getHL()));
            return 8;
        }

        case 0x4F: // LD C, A
        {
            setC(getA());
            return 4;
        }

        case 0x50: // LD D, B
        {
           setD(getB());
           return 4;
        }

        case 0x51: // LD D, C
        {
            setD(getC());
            return 4;
        }

        case 0x52: // LD D, D
        {
            setD(getD());
            return 4;
        }

        case 0x53: // LD D, E
        {
            setD(getE());
            return 4;
        }

        case 0x54: // LD D, H
        {
            setD(getH());
            return 4;
        }

        case 0x55: // LD D, L
        {
            setD(getL());
            return 4;
        }

        case 0x56: // LD D, (HL)
        {
            setD(bus->read(getHL()));
            return 8;
        }

        case 0x57: // LD D, A
        {
            setD(getA());
            return 4;
        }

        case 0x58: // LD E, B
        {
            setE(getB());
            return 4;
        }

        case 0x59: // LD E, C
        {
            setE(getC());
            return 4;
        }

        case 0x5A: // LD E, D
        {
            setE(getD());
            return 4;
        }

        case 0x5B: // LD E, E
        {
            setE(getE());
            return 4;
        }

        case 0x5C: // LD E, H
        {
            setE(getH());
            return 4;
        }

        case 0x5D: // LD E, L
        {
            setE(getL());
            return 4;
        }

        case 0x5E: // LD E, (HL)
        {
            setE(bus->read(getHL()));
            return 8;
        }

        case 0x5F: // LD E, A
        {
            setE(getA());
            return 4;
        }

        case 0x60: // LD H, B
        {
            setH(getB());
            return 4;
        }

        case 0x61: // LD H, C
        {
            setH(getC());
            return 4;
        }

        case 0x62: // LD H, D
        {
            setH(getD());
            return 4;
        }

        case 0x63: // LD H, E
        {
            setH(getE());
            return 4;
        }

        case 0x64: // LD H, H
        {
            setH(getH());
            return 4;
        }

        case 0x65: // LD H, L
        {
            setH(getL());
            return 4;
        }

        case 0x66: // LD H, (HL)
        {
            setH(bus->read(getHL()));
            return 8;
        }

        case 0x67: // LD H, A
        {
            setH(getA());
            return 4;
        }

        case 0x68: // LD L, B
        {
            setL(getB());
            return 4;
        }

        case 0x69: // LD L, C
        {
            setL(getC());
            return 4;
        }

        case 0x6A: // LD L, D
        {
            setL(getD());
            return 4;
        }

        case 0x6B: // LD L, E
        {
            setL(getE());
            return 4;
        }

        case 0x6C: // LD L, H
        {
            setL(getH());
            return 4;
        }

        case 0x6D: // LD L, L
        {
            setL(getL());
            return 4;
        }

        case 0x6E: // LD L, (HL)
        {
            setL(bus->read(getHL()));
            return 8;
        }

        case 0x6F: // LD L, A
        {
            setL(getA());
            return 4;
        }

        case 0x70: // LD (HL), B
        {
            bus->write(getHL(), getB());
            return 8;
        }

        case 0x71: // LD (HL), C
        {
            bus->write(getHL(), getC());
            return 8;
        }

        case 0x72: // LD (HL), D
        {
            bus->write(getHL(), getD());
            return 8;
        }

        case 0x73: // LD (HL), E
        {
            bus->write(getHL(), getE());
            return 8;
        }

        case 0x74: // LD (HL), H
        {
            bus->write(getHL(), getH());
            return 8;
        }

        case 0x75: // LD (HL), L
        {
            bus->write(getHL(), getL());
            return 8;
        }

        case 0x76: // HALT
        {
            halted = true;
            return 4;
        }

        case 0x77: // LD (HL), A
        {
            bus->write(getHL(), getA());
            return 8;
        }

        case 0x78: // LD A,B
        {
            setA(getB());
            return 4;
        }

        case 0x79: // LD A,C
        {
            setA(getC());
            return 4;
        }

        case 0x7A: // LD A,D
        {
            setA(getD());
            return 4;
        }

        case 0x7B: // LD A,E
        {
            setA(getE());
            return 4;
        }

        case 0x7C: // LD A,H
        {
            setA(getH());
            return 4;
        }

        case 0x7D: // LD A,L
        {
            setA(getL());
            return 4;
        }

        case 0x7E: // LD A,(HL)
        {
            setA(bus->read(getHL()));
            return 8;
        }

        case 0x7F: // LD A,A
        {
            setA(getA());
            return 4;
        }

        case 0x80: // ADD A,B
        {
            add8(getB());
            return 4;
        }

        case 0x81: // ADD A,C
        {
            add8(getC());
            return 4;
        }

        case 0x82: // ADD A,D
        {
            add8(getD());
            return 4;
        }

        case 0x83: // ADD A,E
        {
            add8(getE());
            return 4;
        }

        case 0x84: // ADD A,H
        {
            add8(getH());
            return 4;
        }

        case 0x85: // ADD A,L
        {
            add8(getL());
            return 4;
        }

        case 0x86: // ADD A,(HL)
        {
            add8(bus->read(getHL()));
            return 8;
        }

        case 0x87: // ADD A,A
        {
            add8(getA());
            return 4;
        }

        case 0x88: // ADC A, B
        {
            adc8(getB());
            return 4;
        }

        case 0x89: // ADC A, C
        {
            adc8(getC());
            return 4;
        }

        case 0x8A: // ADC A, D
        {
            adc8(getD());
            return 4;
        }

        case 0x8B: // ADC A, E
        {
            adc8(getE());
            return 4;
        }

        case 0x8C: // ADC A, H
        {
            adc8(getH());
            return 4;
        }

        case 0x8D: // ADC A, L
        {
            adc8(getL());
            return 4;
        }

        case 0x8E: // ADC A, (HL)
        {
            adc8(bus->read(getHL()));
            return 8;
        }

        case 0x8F: // ADC A, A
        {
            adc8(getA());
            return 4;
        }

        case 0x90: // SUB A, B
        {
            sub8(getB());
            return 4;
        }

        case 0x91: // SUB A, C
        {
            sub8(getC());
            return 4;
        }

        case 0x92: // SUB A, D
        {
            sub8(getD());
            return 4;
        }

        case 0x93: // SUB A, E
        {
            sub8(getE());
            return 4;
        }

        case 0x94: // SUB A, H
        {
            sub8(getH());
            return 4;
        }

        case 0x95: // SUB A, L
        {
            sub8(getL());
            return 4;
        }

        case 0x96: // SUB A, (HL)
        {
            sub8(bus->read(getHL()));
            return 8;
        }

        case 0x97: // SUB A, A
        {
            sub8(getA());
            return 4;
        }

        case 0x98: // SBC A, B
        {
            sbc8(getB());
            return 4;
        }

        case 0x99: // SBC A, C
        {
            sbc8(getC());
            return 4;
        }

        case 0x9A: // SBC A, D
        {
            sbc8(getD());
            return 4;
        }

        case 0x9B: // SBC A, E
        {
            sbc8(getE());
            return 4;
        }

        case 0x9C: // SBC A, H
        {
            sbc8(getH());
            return 4;
        }

        case 0x9D: // SBC A, L
        {
            sbc8(getL());
            return 4;
        }

        case 0x9E: // SBC A, (HL)
        {
            sbc8(bus->read(getHL()));
            return 8;
        }

        case 0x9F: // SBC A, A
        {
            sbc8(getA());
            return 4;
        }

        case 0xA0: // AND A, B
        {
            and8(getB());
            return 4;
        }

        case 0xA1: // AND A, C
        {
            and8(getC());
            return 4;
        }

        case 0xA2: // AND A, D
        {
            and8(getD());
            return 4;
        }

        case 0xA3: // AND A, E
        {
            and8(getE());
            return 4;
        }

        case 0xA4: // AND A, H
        {
            and8(getH());
            return 4;
        }

        case 0xA5: // AND A, L
        {
            and8(getL());
            return 4;
        }

        case 0xA6: // AND A, (HL)
        {
            and8(bus->read(getHL()));
            return 8;
        }

        case 0xA7: // AND A, A
        {
            and8(getA());
            return 4;
        }

        case 0xA8: // XOR A, B
        {
            xor8(getB());
            return 4;
        }

        case 0xA9: // XOR A, C
        {
            xor8(getC());
            return 4;
        }

        case 0xAA: // XOR A, D
        {
            xor8(getD());
            return 4;
        }

        case 0xAB: // XOR A, E
        {
            xor8(getE());
            return 4;
        }

        case 0xAC: // XOR A, H
        {
            xor8(getH());
            return 4;
        }

        case 0xAD: // XOR A, L
        {
            xor8(getL());
            return 4;
        }

        case 0xAE: // XOR A, (HL)
        {
            xor8(bus->read(getHL()));
            return 8;
        }

        case 0xAF: // XOR A, A
        {
            xor8(getA());
            return 4;
        }

        case 0xB0: // OR A, B
        {
            or8(getB());
            return 4;
        }

        case 0xB1: // OR A, C
        {
            or8(getC());
            return 4;
        }

        case 0xB2: // OR A, D
        {
            or8(getD());
            return 4;
        }

        case 0xB3: // OR A, E
        {
            or8(getE());
            return 4;
        }

        case 0xB4: // OR A, H
        {
            or8(getH());
            return 4;
        }

        case 0xB5: // OR A, L
        {
            or8(getL());
            return 4;
        }

        case 0xB6: // OR A, (HL)
        {
            or8(bus->read(getHL()));
            return 8;
        }

        case 0xB7: // OR A, A
        {
            or8(getA());
            return 4;
        }

        case 0xB8: // CP A, B
        {
            cp8(getB());
            return 4;
        }

        case 0xB9: // CP A, C
        {
            cp8(getC());
            return 4;
        }

        case 0xBA: // CP A, D
        {
            cp8(getD());
            return 4;
        }

        case 0xBB: // CP A, E
        {
            cp8(getE());
            return 4;
        }

        case 0xBC: // CP A, H
        {
            cp8(getH());
            return 4;
        }

        case 0xBD: // CP A, L
        {
            cp8(getL());
            return 4;
        }

        case 0xBE: // CP A, (HL)
        {
            cp8(bus->read(getHL()));
            return 8;
        }

        case 0xBF: // CP A,A
        {
            cp8(getA());
            return 4;
        }

        case 0xC0: // RET NZ
        {
            if (!getFlag(Z))
            {
                setPC(pop16());
                return 20;
            }

            return 8;
        }

        case 0xC1: // POP BC
        {
            setBC(pop16());
            return 12;
        }

        case 0xC2: // JP NZ,u16
        {
            uint16_t address = fetch16();

            if (!getFlag(Z))
            {
                setPC(address);
                return 16;
            }

            return 12;
        }

        case 0xC3: // JP u16
        {
            setPC(fetch16());
            return 16;
        }

        case 0xC4: // CALL NZ,u16
        {
            uint16_t address = fetch16();

            if (!getFlag(Z))
            {
                push16(getPC());
                setPC(address);
                return 24;
            }

            return 12;
        }

        case 0xC5: // PUSH BC
        {
            push16(getBC());
            return 16;
        }

        case 0xC6: // ADD A,u8
        {
            add8(fetch8());
            return 8;
        }

        case 0xC7: // RST 00H
        {
            push16(getPC());
            setPC(0x0000);
            return 16;
        }

        case 0xC8: // RET Z
        {
            if (getFlag(Z))
            {
                ret();
                return 20;
            }

            return 8;
        }

        case 0xC9: // RET
        {
            ret();
            return 16;
        }

        case 0xCA: // JP Z, U16
        {
            uint16_t address = fetch16();

            if (getFlag(Z))
            {
                setPC(address);
                return 16;
            }

            return 12;
        }

        case 0xCB: // PREFIX CB
        {
            uint8_t cbOPCode = fetch8();
            return decodeCB(cbOPCode);
        }

        case 0xCC: // CALL Z, U16
        {
            uint16_t address = fetch16();

            if (getFlag(Z))
            {
                push16(getPC());
                setPC(address);
                return 24;
            }

            return 12;
        }

        case 0xCD: // CALL U16
        {
            uint16_t address = fetch16();

            push16(getPC());
            setPC(address);
            return 24;
        }

        case 0xCE: // ADC A, U8
        {
            adc8(fetch8());
            return 8;
        }

        case 0xCF: // RST 08H
        {
            push16(getPC());
            setPC(0x0008);
            return 16;
        }

        case 0xD0: // RET NC
        {
            if (!getFlag(C))
            {
                ret();
                return 20;
            }

            return 8;
        }

        case 0xD1: // POP DE
        {
            setDE(pop16());
            return 12;
        }

        case 0xD2: // JP NC,u16
        {
            uint16_t address = fetch16();

            if (!getFlag(C))
            {
                setPC(address);
                return 16;
            }

            return 12;
        }

        case 0xD4: // CALL NC,u16
        {
            uint16_t address = fetch16();

            if (!getFlag(C))
            {
                push16(getPC());
                setPC(address);
                return 24;
            }

            return 12;
        }

        case 0xD5: // PUSH DE
        {
            push16(getDE());
            return 16;
        }

        case 0xD6: // SUB A,u8
        {
            sub8(fetch8());
            return 8;
        }

        case 0xD7: // RST 10H
        {
            push16(getPC());
            setPC(0x0010);
            return 16;
        }

        case 0xD8: // RET C
        {
            if (getFlag(C))
            {
                ret();
                return 20;
            }

            return 8;
        }

        case 0xD9: // RETI
        {
            ret();
            IME = true;
            imeEnablePending = false;
            return 16;
        }

        case 0xDA: // JP C,u16
        {
            uint16_t address = fetch16();

            if (getFlag(C))
            {
                setPC(address);
                return 16;
            }

            return 12;
        }

        case 0xDC: // CALL C,u16
        {
            uint16_t address = fetch16();

            if (getFlag(C))
            {
                push16(getPC());
                setPC(address);
                return 24;
            }

            return 12;
        }

        case 0xDE: // SBC A,u8
        {
            sbc8(fetch8());
            return 8;
        }

        case 0xDF: // RST 18H
        {
            push16(getPC());
            setPC(0x0018);
            return 16;
        }

        case 0xE0: // LDH (u8),A
        {
            uint16_t address = uint16_t(0xFF00 + fetch8());
            bus->write(address, getA());
            return 12;
        }

        case 0xE1: // POP HL
        {
            setHL(pop16());
            return 12;
        }

        case 0xE2: // LD (C),A
        {
            uint16_t address = uint16_t(0xFF00 + getC());
            bus->write(address, getA());
            return 8;
        }

        case 0xE5: // PUSH HL
        {
            push16(getHL());
            return 16;
        }

        case 0xE6: // AND A,u8
        {
            and8(fetch8());
            return 8;
        }

        case 0xE7: // RST 20H
        {
            push16(getPC());
            setPC(0x0020);
            return 16;
        }

        case 0xE8: // ADD SP,i8
        {
            int8_t offset = int8_t(fetch8());
            uint16_t sp = getSP();
            uint16_t unsignedOffset = uint16_t(uint8_t(offset));
            uint16_t result = uint16_t(sp + offset);

            setFlag(Z, false);
            setFlag(N, false);
            setFlag(H, ((sp & 0x000F) + (unsignedOffset & 0x000F)) > 0x000F);
            setFlag(C, ((sp & 0x00FF) + (unsignedOffset & 0x00FF)) > 0x00FF);

            setSP(result);
            return 16;
        }

        case 0xE9: // JP HL
        {
            setPC(getHL());
            return 4;
        }

        case 0xEA: // LD (u16),A
        {
            uint16_t address = fetch16();
            bus->write(address, getA());
            return 16;
        }

        case 0xEE: // XOR A,u8
        {
            xor8(fetch8());
            return 8;
        }

        case 0xEF: // RST 28H
        {
            push16(getPC());
            setPC(0x0028);
            return 16;
        }

        case 0xF0: // LDH A,(u8)
        {
            uint16_t address = uint16_t(0xFF00 + fetch8());
            setA(bus->read(address));
            return 12;
        }

        case 0xF1: // POP AF
        {
            setAF(pop16());
            return 12;
        }

        case 0xF2: // LD A, (FF00 + C)
        {
            uint16_t address = uint16_t(0xFF00 + getC());
            setA(bus->read(address));
            return 8;
        }

        case 0xF3: // DI
        {
            IME = false;
            imeEnablePending = false;
            return 4;
        }

        case 0xF5: // PUSH AF
        {
            push16(getAF());
            return 16;
        }

        case 0xF6: // OR A, U8
        {
            or8(fetch8());
            return 8;
        }

        case 0xF7: // RST 30H
        {
            push16(getPC());
            setPC(0x0030);
            return 16;
        }

        case 0xF8: // LD HL,SP+i8
        {
            int8_t offset = int8_t(fetch8());

            uint16_t sp = getSP();
            uint16_t unsignedOffset = uint16_t(uint8_t(offset));
            uint16_t result = uint16_t(sp + offset);

            setFlag(Z, false);
            setFlag(N, false);
            setFlag(H, ((sp & 0x000F) + (unsignedOffset & 0x000F)) > 0x000F);
            setFlag(C, ((sp & 0x00FF) + (unsignedOffset & 0x00FF)) > 0x00FF);

            setHL(result);
            return 12;
        }

        case 0xF9: // LD SP, HL
        {
            setSP(getHL());
            return 8;
        }

        case 0xFA: // LD A, (U16)
        {
            uint16_t address = fetch16();
            setA(bus->read(address));
            return 16;
        }

        case 0xFB: // EI
        {
            imeEnablePending = true;
            return 4;
        }

        case 0xFE: // CP A,u8
        {
            cp8(fetch8());
            return 8;
        }

        case 0xFF: // RST 38H
        {
            push16(getPC());
            setPC(0x0038);
            return 16;
        }

        default:
            return illegalOpcode(opcode);
    }
}

int CPU::decodeCB(uint8_t opcode)
{
    switch (opcode)
    {
        default:
            return illegalOpcodeCB(opcode);
    }
}

void CPU::setFlag(Flags flag, bool set)
{
    uint8_t f = getF();

    if (set)
        f |= flag;
    else
        f &= ~flag;

    setF(f);
}

bool CPU::getFlag(Flags flag) const
{
    return (getF() & flag) != 0;
}

void CPU::push16(uint16_t value)
{
    setSP(uint16_t(getSP() - 1));
    bus->write(getSP(), uint8_t(value >> 8));      // high byte

    setSP(uint16_t(getSP() - 1));
    bus->write(getSP(), uint8_t(value & 0x00FF));  // low byte
}

uint16_t CPU::pop16()
{
    uint8_t lo = bus->read(getSP());
    setSP(uint16_t(getSP() + 1));

    uint8_t hi = bus->read(getSP());
    setSP(uint16_t(getSP() + 1));

    return uint16_t(lo) | (uint16_t(hi) << 8);
}

void CPU::add8(uint8_t value)
{
    uint8_t a = getA();
    uint16_t result = uint16_t(a) + uint16_t(value);

    setA(uint8_t(result));

    setFlag(Z, uint8_t(result) == 0);
    setFlag(N, false);
    setFlag(H, ((a & 0x0F) + (value & 0x0F)) > 0x0F);
    setFlag(C, result > 0xFF);
}

void CPU::adc8(uint8_t value)
{
    uint8_t a = getA();
    uint8_t carry = getFlag(C) ? 1 : 0;

    uint16_t result = uint16_t(a) + uint16_t(value) + carry;

    setA(uint8_t(result));

    setFlag(Z, uint8_t(result) == 0);
    setFlag(N, false);
    setFlag(H, ((a & 0x0F) + (value & 0x0F) + carry) > 0x0F);
    setFlag(C, result > 0xFF);
}

void CPU::sub8(uint8_t value)
{
    uint8_t a = getA();
    uint16_t result = uint16_t(a) - uint16_t(value);

    setA(uint8_t(result));

    setFlag(Z, uint8_t(result) == 0);
    setFlag(N, true);
    setFlag(H, (a & 0x0F) < (value & 0x0F));
    setFlag(C, a < value);
}

void CPU::sbc8(uint8_t value)
{
    uint8_t a = getA();
    uint8_t carry = getFlag(C) ? 1 : 0;

    uint16_t result = uint16_t(a) - uint16_t(value) - carry;

    setA(uint8_t(result));

    setFlag(Z, uint8_t(result) == 0);
    setFlag(N, true);
    setFlag(H, (a & 0x0F) < ((value & 0x0F) + carry));
    setFlag(C, uint16_t(a) < uint16_t(value) + carry);
}

void CPU::and8(uint8_t value)
{
    uint8_t result = getA() & value;

    setA(result);

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, true);
    setFlag(C, false);
}

void CPU::xor8(uint8_t value)
{
    uint8_t result = getA() ^ value;

    setA(result);

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, false);
}

void CPU::or8(uint8_t value)
{
    uint8_t result = getA() | value;

    setA(result);

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, false);
}

void CPU::cp8(uint8_t value)
{
    uint8_t a = getA();
    uint8_t result = uint8_t(a - value);

    setFlag(Z, result == 0);
    setFlag(N, true);
    setFlag(H, (a & 0x0F) < (value & 0x0F));
    setFlag(C, a < value);
}



uint8_t CPU::inc8(uint8_t value)
{
    uint8_t result = uint8_t(value + 1);

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, (value & 0x0F) == 0x0F);

    return result;
}

uint8_t CPU::dec8(uint8_t value)
{
    uint8_t result = uint8_t(value - 1);

    setFlag(Z, result == 0);
    setFlag(N, true);
    setFlag(H, (value & 0x0F) == 0x00);

    return result;
}

void CPU::rlca()
{
    const uint8_t a = getA();

    const bool carry = (a & 0x80) != 0;
    const uint8_t result = uint8_t((a << 1) | (carry ? 1 : 0));

    setA(result);

    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, carry);
}

void CPU::rrca()
{
    const uint8_t a = getA();

    const bool carry = (a & 0x01) != 0;
    const uint8_t result = uint8_t((a >> 1) | (carry ? 0x80 : 0x00));

    setA(result);

    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, carry);
}

void CPU::rla()
{
    const uint8_t a = getA();

    const bool oldCarry = getFlag(C);
    const bool newCarry = (a & 0x80) != 0;

    const uint8_t result = uint8_t((a << 1) | (oldCarry ? 0x01 : 0x00));

    setA(result);

    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, newCarry);
}

void CPU::rra()
{
    const uint8_t a = getA();

    const bool oldCarry = getFlag(C);
    const bool newCarry = (a & 0x01) != 0;

    const uint8_t result = uint8_t((a >> 1) | (oldCarry ? 0x80 : 0x00));

    setA(result);

    setFlag(Z, false);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, newCarry);
}

void CPU::addHL(uint16_t value)
{
    uint16_t hl = getHL();
    uint32_t result = uint32_t(hl) + uint32_t(value);

    setFlag(N, false);
    setFlag(H, ((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF);
    setFlag(C, result > 0xFFFF);

    setHL(uint16_t(result));
}

void CPU::daa()
{
    uint8_t a = getA();
    uint8_t correction = 0;
    bool setCarry = false;

    if (!getFlag(N)) // after addition
    {
        if (getFlag(H) || (a & 0x0F) > 0x09)
            correction |= 0x06;

        if (getFlag(C) || a > 0x99)
        {
            correction |= 0x60;
            setCarry = true;
        }

        a = uint8_t(a + correction);
    }
    else // after subtraction
    {
        if (getFlag(H))
            correction |= 0x06;

        if (getFlag(C))
            correction |= 0x60;

        a = uint8_t(a - correction);

        // On subtraction, C remains whatever it already was.
        setCarry = getFlag(C);
    }

    setA(a);

    setFlag(Z, a == 0);
    setFlag(H, false);
    setFlag(C, setCarry);

    // N is unchanged
}

void CPU::cpl()
{
    setA(uint8_t(~getA()));

    setFlag(N, true);
    setFlag(H, true);

    // Z unchanged
    // C unchanged
}

void CPU::scf()
{
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, true);

    // Z unchanged
}

void CPU::ccf()
{
    bool oldCarry = getFlag(C);

    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, !oldCarry);

    // Z unchanged
}

uint8_t CPU::rlc8(uint8_t value)
{
    bool carry = (value & 0x80) != 0;
    uint8_t result = uint8_t((value << 1) | (carry ? 1 : 0));

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, carry);

    return result;
}

uint8_t CPU::rrc8(uint8_t value)
{
    bool carry = (value & 0x01) != 0;
    uint8_t result = uint8_t((value >> 1) | (carry ? 0x80 : 0x00));

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, carry);

    return result;
}

uint8_t CPU::rl8(uint8_t value)
{
    bool oldCarry = getFlag(C);
    bool newCarry = (value & 0x80) != 0;

    uint8_t result = uint8_t((value << 1) | (oldCarry ? 1 : 0));

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, newCarry);

    return result;
}

uint8_t CPU::rr8(uint8_t value)
{
    bool oldCarry = getFlag(C);
    bool newCarry = (value & 0x01) != 0;

    uint8_t result = uint8_t((value >> 1) | (oldCarry ? 0x80 : 0x00));

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, newCarry);

    return result;
}

uint8_t CPU::sla8(uint8_t value)
{
    bool carry = (value & 0x80) != 0;
    uint8_t result = uint8_t(value << 1);

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, carry);

    return result;
}

uint8_t CPU::sra8(uint8_t value)
{
    bool carry = (value & 0x01) != 0;
    uint8_t result = uint8_t((value >> 1) | (value & 0x80));

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, carry);

    return result;
}

uint8_t CPU::swap8(uint8_t value)
{
    uint8_t result = uint8_t((value >> 4) | (value << 4));

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, false);

    return result;
}

uint8_t CPU::srl8(uint8_t value)
{
    bool carry = (value & 0x01) != 0;
    uint8_t result = uint8_t(value >> 1);

    setFlag(Z, result == 0);
    setFlag(N, false);
    setFlag(H, false);
    setFlag(C, carry);

    return result;
}

void CPU::bit8(uint8_t bit, uint8_t value)
{
    bool bitIsZero = (value & (1u << bit)) == 0;

    setFlag(Z, bitIsZero);
    setFlag(N, false);
    setFlag(H, true);
    // C unchanged
}

uint8_t CPU::res8(uint8_t bit, uint8_t value)
{
    return uint8_t(value & ~(1u << bit));
}

uint8_t CPU::set8(uint8_t bit, uint8_t value)
{
    return uint8_t(value | (1u << bit));
}

int CPU::illegalOpcode(uint8_t opcode)
{
    std::cerr << "Unhandled opcode: $"
              << std::hex << int(opcode)
              << " at PC=$" << int(PC - 1)
              << std::dec << "\n";

    halted = true;
    return 4;
}

int CPU::illegalOpcodeCB(uint8_t opcode)
{
    std::cerr << "Unhandled CB opcode: $"
              << std::hex << int(opcode)
              << " at PC=$" << int(PC - 1)
              << std::dec << "\n";

    halted = true;
    return 4;
}
