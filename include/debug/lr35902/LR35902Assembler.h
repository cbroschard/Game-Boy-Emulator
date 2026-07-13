// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef LR35902ASSEMBLER_H
#define LR35902ASSEMBLER_H

#include <cstdint>
#include <string>
#include <vector>

#include "debug/lr35902/OpcodeLR35902.h"

class LR35902Assembler
{
    public:
        struct AssembledInstruction
        {
            uint16_t startAddress = 0;
            uint16_t nextAddress = 0;

            uint8_t opcode = 0;
            LR35902OpcodePage page = LR35902OpcodePage::Main;

            std::vector<uint8_t> bytes;
        };

        LR35902Assembler();
        ~LR35902Assembler();

        AssembledInstruction assembleLine(
            const std::string& line,
            uint16_t address);

    private:
        struct ParsedOperand
        {
            LR35902OperandMode mode = LR35902OperandMode::None;

            uint16_t value = 0;
            bool hasValue = false;
        };

        ParsedOperand parseOperand(
            const std::string& operand,
            const std::string& mnemonic,
            int operandIndex);

        const LR35902InstructionInfo& lookupInstruction(
            const std::string& mnemonic,
            const ParsedOperand& operand1,
            const ParsedOperand& operand2) const;

        static std::string trim(const std::string& text);
        static std::string toUpper(std::string text);

        static std::vector<std::string> splitOperands(
            const std::string& operandText);

        static uint16_t parseNumber(const std::string& text);

        static void appendImmediateBytes(
            std::vector<uint8_t>& bytes,
            const LR35902InstructionInfo& info,
            const ParsedOperand& operand1,
            const ParsedOperand& operand2,
            uint16_t instructionAddress);
};

#endif // LR35902ASSEMBLER_H
