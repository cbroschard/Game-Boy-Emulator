// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef CPU_H
#define CPU_H

#include <cstdint>

class Bus;

struct lr35902CPUState
{
    uint16_t SP = 0;
    uint16_t PC = 0;

    uint16_t AF = 0;
    uint16_t BC = 0;
    uint16_t DE = 0;
    uint16_t HL = 0;

    uint8_t  A  = 0;
    uint8_t  F  = 0;
    uint8_t  B  = 0;
    uint8_t  C  = 0;
    uint8_t  D  = 0;
    uint8_t  E  = 0;
    uint8_t  H  = 0;
    uint8_t  L  = 0;

    uint64_t cycles = 0;

    bool halted = false;
    bool stopped = false;
    bool IME = false;
    bool imeEnablePending = false;
};

class CPU
{
    public:
        CPU();
        virtual ~CPU();

        inline void attachBusInstance(Bus* bus) { this->bus = bus; }

        void reset();
        int step();

        inline bool hasBus() const { return bus ? 1 : 0; }

        // ML Monitor API
        inline uint16_t getAF() const { return AF; }
        inline uint16_t getBC() const { return BC; }
        inline uint16_t getDE() const { return DE; }
        inline uint16_t getHL() const { return HL; }

        inline uint16_t getSP() const { return SP; }
        inline uint16_t getPC() const { return PC; }

        inline void setSP(uint16_t value) { SP = value; }
        inline void setPC(uint16_t value) { PC = value; }

        lr35902CPUState getCPUState() const;

    protected:

    private:
        Bus* bus;

        // Registers
        uint16_t AF; // A = high byte, F = low byte
        uint16_t BC; // B = high byte, C = low byte
        uint16_t DE; // D = high byte, E = low byte
        uint16_t HL; // H = high byte, L = low byte
        uint16_t SP; // Stack pointer
        uint16_t PC; // Program counter

        enum Flags : uint8_t
        {
            C = 1u << 4,    // Carry
            H = 1u << 5,    // Half Carry (BCD)
            N = 1u << 6,    // Subtraction (BCD)
            Z = 1u << 7     // Zero
        };

        int cycles;

        bool halted;
        bool stopped;

        // Interrupt state
        bool IME;              // Interrupt Master Enable
        bool imeEnablePending; // EI enables IME after the following instruction

        // Register getters
        inline uint8_t getA() const { return uint8_t(AF >> 8); }
        inline uint8_t getF() const { return uint8_t(AF & 0xF0); }

        inline uint8_t getB() const { return uint8_t(BC >> 8); }
        inline uint8_t getC() const { return uint8_t(BC & 0x00FF); }

        inline uint8_t getD() const { return uint8_t(DE >> 8); }
        inline uint8_t getE() const { return uint8_t(DE & 0x00FF); }

        inline uint8_t getH() const { return uint8_t(HL >> 8); }
        inline uint8_t getL() const { return uint8_t(HL & 0x00FF); }

        // Register setters
        inline void setA(uint8_t value) { AF = (AF & 0x00FF) | (uint16_t(value) << 8); }
        inline void setF(uint8_t value) { AF = (AF & 0xFF00) | (value & 0xF0); }
        inline void setB(uint8_t value) { BC = (BC & 0x00FF) | (uint16_t(value) << 8); }
        inline void setC(uint8_t value) { BC = (BC & 0xFF00) | value; }
        inline void setD(uint8_t value) { DE = (DE & 0x00FF) | (uint16_t(value) << 8); }
        inline void setE(uint8_t value) { DE = (DE & 0xFF00) | value; }
        inline void setH(uint8_t value) { HL = (HL & 0x00FF) | (uint16_t(value) << 8); }
        inline void setL(uint8_t value) { HL = (HL & 0xFF00) | value; }

        inline void setAF(uint16_t value) { AF = value & 0xFFF0; }
        inline void setBC(uint16_t value) { BC = value; }
        inline void setDE(uint16_t value) { DE = value; }
        inline void setHL(uint16_t value) { HL = value; }

        int decodeExecute(uint8_t opcode);
        int decodeCB(uint8_t opcode);

        uint8_t fetch8();
        uint16_t fetch16();

        // Flag helpers
        void setFlag(Flags flag, bool set);
        bool getFlag(Flags flag) const;

        // Stack helpers
        void push16(uint16_t value);
        uint16_t pop16();

        int serviceInterrupts();

        inline void ret() { setPC(pop16()); }

        // opcode helpers
        void add8(uint8_t value);
        void adc8(uint8_t value);
        void sub8(uint8_t value);
        void sbc8(uint8_t value);

        void and8(uint8_t value);
        void xor8(uint8_t value);
        void or8(uint8_t value);
        void cp8(uint8_t value);

        uint8_t inc8(uint8_t value);
        uint8_t dec8(uint8_t value);

        inline uint16_t inc16(uint16_t value) { return uint16_t(value + 1); }
        inline uint16_t dec16(uint16_t value) { return uint16_t(value - 1); }

        void rlca();
        void rrca();
        void rla();
        void rra();

        void addHL(uint16_t value);

        void daa();

        void cpl();

        void scf();
        void ccf();

        // CB Opcode helpers
        uint8_t rlc8(uint8_t value);
        uint8_t rrc8(uint8_t value);
        uint8_t rl8(uint8_t value);
        uint8_t rr8(uint8_t value);

        uint8_t sla8(uint8_t value);
        uint8_t sra8(uint8_t value);
        uint8_t swap8(uint8_t value);
        uint8_t srl8(uint8_t value);

        void bit8(uint8_t bit, uint8_t value);
        uint8_t res8(uint8_t bit, uint8_t value);
        uint8_t set8(uint8_t bit, uint8_t value);

        // Illegal opcode catch
        int illegalOpcode(uint8_t opcode);
        int illegalOpcodeCB(uint8_t opcode);
};

#endif // CPU_H
