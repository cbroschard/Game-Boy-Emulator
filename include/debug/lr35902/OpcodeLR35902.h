// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef OPCODELR35902_H_INCLUDED
#define OPCODELR35902_H_INCLUDED

#include <cstdint>
#include <string>
#include <unordered_map>

enum class LR35902OpcodePage
{
    Main,
    CB
};

enum class LR35902OperandMode
{
    None,

    // 8-bit registers
    A,
    B,
    C,
    D,
    E,
    H,
    L,

    // 16-bit registers
    AF,
    BC,
    DE,
    HL,
    SP,

    // Conditions
    NZ,
    Z,
    NC,
    CFlag,

    // Memory references through registers
    AddrBC,     // (BC)
    AddrDE,     // (DE)
    AddrHL,     // (HL)
    AddrHLInc,  // (HL+) / HLI
    AddrHLDec,  // (HL-) / HLD

    AddrC,      // ($FF00 + C)

    // Immediate operands
    Imm8,       // u8 / d8
    Imm16,      // u16 / d16
    Rel8,       // i8 / signed relative offset

    // Memory references through immediate values
    AddrImm16,          // (u16)
    AddrImm8,           // ($FF00 + u8)
    SignedImm8,         // Signed 8-bit immediate value
    SPPlusSignedImm8,   // SP plus signed 8-bit immediate offset

    // Constants / restart vectors
    Rst00,
    Rst08,
    Rst10,
    Rst18,
    Rst20,
    Rst28,
    Rst30,
    Rst38,

    // Bit instruction operand
    Bit0,
    Bit1,
    Bit2,
    Bit3,
    Bit4,
    Bit5,
    Bit6,
    Bit7
};

struct LR35902InstructionInfo
{
    const char* mnemonic;
    LR35902OperandMode operand1;
    LR35902OperandMode operand2;
    uint8_t length;
    uint8_t opcode;
    LR35902OpcodePage page;
};

struct LR35902MnemonicKey
{
    std::string mnemonic;
    LR35902OperandMode operand1;
    LR35902OperandMode operand2;

    bool operator==(const LR35902MnemonicKey& other) const noexcept
    {
        return mnemonic == other.mnemonic &&
               operand1 == other.operand1 &&
               operand2 == other.operand2;
    }
};

struct LR35902MnemonicKeyHash
{
    std::size_t operator()(const LR35902MnemonicKey& key) const noexcept
    {
        const std::size_t h1 =
            std::hash<std::string>{}(key.mnemonic);

        const std::size_t h2 =
            std::hash<int>{}(static_cast<int>(key.operand1));

        const std::size_t h3 =
            std::hash<int>{}(static_cast<int>(key.operand2));

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

extern const LR35902InstructionInfo LR35902_MAIN_OPCODES[256];
extern const LR35902InstructionInfo LR35902_CB_OPCODES[256];

extern const std::unordered_map<LR35902MnemonicKey, LR35902InstructionInfo, LR35902MnemonicKeyHash> LR35902_MNEMONIC_TO_OPCODE;

#endif // OPCODELR35902_H_INCLUDED
