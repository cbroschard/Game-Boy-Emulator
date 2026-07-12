// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef LR35902DISASSEMBLER_H
#define LR35902DISASSEMBLER_H

#include <cstdint>
#include <functional>
#include <string>
#include "debug/lr35902/OpcodeLR35902.h"

struct LR35902DisassembledInstruction
{
    uint16_t address = 0;
    uint8_t bytes[4] = {0, 0, 0, 0};
    uint8_t size = 0;
    std::string text;
};

class LR35902Disassembler
{
    public:
        using ReadByteCallback = std::function<uint8_t(uint16_t)>;

        explicit LR35902Disassembler(ReadByteCallback reader);
        virtual ~LR35902Disassembler();

        LR35902DisassembledInstruction disassemble(uint16_t address) const;

    protected:

    private:
        ReadByteCallback readByte;

        uint8_t read(uint16_t address) const;

        LR35902DisassembledInstruction disassembleWithTable(
            uint16_t address,
            const LR35902InstructionInfo& info) const;

        std::string formatInstruction(
            const LR35902InstructionInfo& info,
            uint16_t address,
            const uint8_t* bytes,
            uint8_t size) const;

        std::string formatOperand(
            LR35902OperandMode operand,
            uint16_t address,
            const uint8_t* bytes,
            uint8_t size) const;

        static std::string hex8(uint8_t value);
        static std::string hex16(uint16_t value);
};

#endif // LR35902DISASSEMBLER_H
