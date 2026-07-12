// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "debug/lr35902/LR35902Disassembler.h"
#include <iomanip>
#include <sstream>

LR35902Disassembler::LR35902Disassembler(ReadByteCallback reader) :
    readByte(reader)
{

}

LR35902Disassembler::~LR35902Disassembler()
{

}

uint8_t LR35902Disassembler::read(uint16_t address) const
{
    return readByte(address);
}

std::string LR35902Disassembler::hex8(uint8_t value)
{
    std::ostringstream out;
    out << "$"
        << std::uppercase
        << std::hex
        << std::setw(2)
        << std::setfill('0')
        << static_cast<int>(value);

    return out.str();
}

std::string LR35902Disassembler::hex16(uint16_t value)
{
    std::ostringstream out;
    out << "$"
        << std::uppercase
        << std::hex
        << std::setw(4)
        << std::setfill('0')
        << static_cast<int>(value);

    return out.str();
}

LR35902DisassembledInstruction LR35902Disassembler::disassemble(uint16_t address) const
{
     if (!readByte)
    {
        LR35902DisassembledInstruction result;
        result.address = address;
        result.size = 0;
        result.text = "???";
        return result;
    }

    const uint8_t opcode = read(address);

    if (opcode == 0xCB)
    {
        const uint8_t cbOpcode =
            read(static_cast<uint16_t>(address + 1));

        return disassembleWithTable(
            address,
            LR35902_CB_OPCODES[cbOpcode]);
    }

    return disassembleWithTable(
        address,
        LR35902_MAIN_OPCODES[opcode]);
}

LR35902DisassembledInstruction
LR35902Disassembler::disassembleWithTable(
    uint16_t address,
    const LR35902InstructionInfo& info) const
{
    LR35902DisassembledInstruction result;

    result.address = address;
    result.size = info.length;

    if (result.size > sizeof(result.bytes))
        result.size = sizeof(result.bytes);

    for (uint8_t i = 0; i < result.size; ++i)
    {
        result.bytes[i] =
            read(static_cast<uint16_t>(address + i));
    }

    result.text = formatInstruction(
        info,
        address,
        result.bytes,
        result.size);

    return result;
}

std::string LR35902Disassembler::formatInstruction(
    const LR35902InstructionInfo& info,
    uint16_t address,
    const uint8_t* bytes,
    uint8_t size) const
{
    std::ostringstream out;

    out << info.mnemonic;

    if (info.operand1 != LR35902OperandMode::None)
    {
        out << " "
            << formatOperand(
                info.operand1,
                address,
                bytes,
                size);
    }

    if (info.operand2 != LR35902OperandMode::None)
    {
        out << ","
            << formatOperand(
                info.operand2,
                address,
                bytes,
                size);
    }

    return out.str();
}

std::string LR35902Disassembler::formatOperand(
    LR35902OperandMode operand,
    uint16_t address,
    const uint8_t* bytes,
    uint8_t size) const
{
    switch (operand)
    {
        case LR35902OperandMode::None:
            return "";

        // 8-bit registers
        case LR35902OperandMode::A:
            return "A";

        case LR35902OperandMode::B:
            return "B";

        case LR35902OperandMode::C:
            return "C";

        case LR35902OperandMode::D:
            return "D";

        case LR35902OperandMode::E:
            return "E";

        case LR35902OperandMode::H:
            return "H";

        case LR35902OperandMode::L:
            return "L";

        // 16-bit registers
        case LR35902OperandMode::AF:
            return "AF";

        case LR35902OperandMode::BC:
            return "BC";

        case LR35902OperandMode::DE:
            return "DE";

        case LR35902OperandMode::HL:
            return "HL";

        case LR35902OperandMode::SP:
            return "SP";

        // Conditions
        case LR35902OperandMode::NZ:
            return "NZ";

        case LR35902OperandMode::Z:
            return "Z";

        case LR35902OperandMode::NC:
            return "NC";

        case LR35902OperandMode::CFlag:
            return "C";

        // Memory references through registers
        case LR35902OperandMode::AddrBC:
            return "(BC)";

        case LR35902OperandMode::AddrDE:
            return "(DE)";

        case LR35902OperandMode::AddrHL:
            return "(HL)";

        case LR35902OperandMode::AddrHLInc:
            return "(HL+)";

        case LR35902OperandMode::AddrHLDec:
            return "(HL-)";

        case LR35902OperandMode::AddrC:
            return "($FF00+C)";

        // Immediate 8-bit value
        case LR35902OperandMode::Imm8:
        {
            if (size < 2)
                return "<?>";

            return hex8(bytes[1]);
        }

        // Immediate 16-bit value, stored little-endian
        case LR35902OperandMode::Imm16:
        {
            if (size < 3)
                return "<?>";

            const uint16_t value =
                static_cast<uint16_t>(bytes[1]) |
                static_cast<uint16_t>(
                    static_cast<uint16_t>(bytes[2]) << 8);

            return hex16(value);
        }

        // Signed relative branch target
        case LR35902OperandMode::Rel8:
        {
            if (size < 2)
                return "<?>";

            const int8_t displacement =
                static_cast<int8_t>(bytes[1]);

            const uint16_t nextAddress =
                static_cast<uint16_t>(address + size);

            const uint16_t target =
                static_cast<uint16_t>(
                    static_cast<int32_t>(nextAddress) +
                    static_cast<int32_t>(displacement));

            return hex16(target);
        }

        // Memory at a 16-bit immediate address
        case LR35902OperandMode::AddrImm16:
        {
            if (size < 3)
                return "(<?>)";

            const uint16_t value =
                static_cast<uint16_t>(bytes[1]) |
                static_cast<uint16_t>(
                    static_cast<uint16_t>(bytes[2]) << 8);

            return "(" + hex16(value) + ")";
        }

        // High-memory address: $FF00 + immediate byte
        case LR35902OperandMode::AddrImm8:
        {
            if (size < 2)
                return "(<?>)";

            const uint16_t value =
                static_cast<uint16_t>(
                    0xFF00u +
                    static_cast<uint16_t>(bytes[1]));

            return "(" + hex16(value) + ")";
        }

        // Signed immediate used by ADD SP,e8
        case LR35902OperandMode::SignedImm8:
        {
            if (size < 2)
                return "<?>";

            const int value =
                static_cast<int>(
                    static_cast<int8_t>(bytes[1]));

            if (value >= 0)
                return "+" + std::to_string(value);

            return std::to_string(value);
        }

        // SP plus signed immediate used by LD HL,SP+e8
        case LR35902OperandMode::SPPlusSignedImm8:
        {
            if (size < 2)
                return "SP+<?>";

            const int value =
                static_cast<int>(
                    static_cast<int8_t>(bytes[1]));

            if (value >= 0)
                return "SP+" + std::to_string(value);

            return "SP" + std::to_string(value);
        }

        // Restart vectors
        case LR35902OperandMode::Rst00:
            return "$00";

        case LR35902OperandMode::Rst08:
            return "$08";

        case LR35902OperandMode::Rst10:
            return "$10";

        case LR35902OperandMode::Rst18:
            return "$18";

        case LR35902OperandMode::Rst20:
            return "$20";

        case LR35902OperandMode::Rst28:
            return "$28";

        case LR35902OperandMode::Rst30:
            return "$30";

        case LR35902OperandMode::Rst38:
            return "$38";

        // Bit numbers
        case LR35902OperandMode::Bit0:
            return "0";

        case LR35902OperandMode::Bit1:
            return "1";

        case LR35902OperandMode::Bit2:
            return "2";

        case LR35902OperandMode::Bit3:
            return "3";

        case LR35902OperandMode::Bit4:
            return "4";

        case LR35902OperandMode::Bit5:
            return "5";

        case LR35902OperandMode::Bit6:
            return "6";

        case LR35902OperandMode::Bit7:
            return "7";
    }

    return "<?>";
}
