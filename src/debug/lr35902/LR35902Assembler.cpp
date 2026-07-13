// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "debug/lr35902/LR35902Assembler.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

LR35902Assembler::LR35902Assembler() = default;

LR35902Assembler::~LR35902Assembler() = default;

LR35902Assembler::AssembledInstruction LR35902Assembler::assembleLine(const std::string& line, uint16_t address)
{
    const std::string cleanedLine = trim(line);

    if (cleanedLine.empty())
        throw std::runtime_error("Cannot assemble an empty line.");

    std::istringstream input(cleanedLine);

    std::string mnemonic;
    input >> mnemonic;

    mnemonic = toUpper(mnemonic);

    std::string operandText;
    std::getline(input, operandText);
    operandText = trim(operandText);

    const std::vector<std::string> operands =
        splitOperands(operandText);

    if (operands.size() > 2)
    {
        throw std::runtime_error(
            "Too many operands for instruction: " + line);
    }

    ParsedOperand operand1;
    ParsedOperand operand2;

    if (!operands.empty())
    {
        operand1 = parseOperand(
            operands[0],
            mnemonic,
            0);
    }

    if (operands.size() >= 2)
       operand2 = parseOperand(operands[1], mnemonic, 1);

    const LR35902InstructionInfo& info = lookupInstruction(mnemonic, operand1, operand2);

    std::vector<uint8_t> bytes;

    if (info.page == LR35902OpcodePage::CB)
        bytes.push_back(0xCB);

    bytes.push_back(info.opcode);

    appendImmediateBytes(bytes, info, operand1, operand2, address);

    if (bytes.size() != info.length)
    {
        throw std::runtime_error(
            "Internal assembler error: encoded size does not match "
            "opcode table length for " +
            mnemonic);
    }

    AssembledInstruction result;

    result.startAddress = address;
    result.nextAddress =
        static_cast<uint16_t>(
            address + bytes.size());

    result.opcode = info.opcode;
    result.page = info.page;
    result.bytes = std::move(bytes);

    return result;
}

LR35902Assembler::ParsedOperand
LR35902Assembler::parseOperand(
    const std::string& operand,
    const std::string& mnemonic,
    int operandIndex)
{
    ParsedOperand result;

    std::string op = toUpper(trim(operand));

    op.erase(
        std::remove_if(
            op.begin(),
            op.end(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            }),
        op.end());

    if (op.empty())
        return result;

    // 8-bit registers
    if (op == "A")
        result.mode = LR35902OperandMode::A;
    else if (op == "B")
        result.mode = LR35902OperandMode::B;
    else if (op == "C")
    {
        // C can be a register or a condition.
        if (mnemonic == "JR" ||
            mnemonic == "JP" ||
            mnemonic == "CALL" ||
            mnemonic == "RET")
        {
            result.mode = LR35902OperandMode::CFlag;
        }
        else
        {
            result.mode = LR35902OperandMode::C;
        }
    }
    else if (op == "D")
        result.mode = LR35902OperandMode::D;
    else if (op == "E")
        result.mode = LR35902OperandMode::E;
    else if (op == "H")
        result.mode = LR35902OperandMode::H;
    else if (op == "L")
        result.mode = LR35902OperandMode::L;

    // 16-bit registers
    else if (op == "AF")
        result.mode = LR35902OperandMode::AF;
    else if (op == "BC")
        result.mode = LR35902OperandMode::BC;
    else if (op == "DE")
        result.mode = LR35902OperandMode::DE;
    else if (op == "HL")
        result.mode = LR35902OperandMode::HL;
    else if (op == "SP")
        result.mode = LR35902OperandMode::SP;

    // Conditions
    else if (op == "NZ")
        result.mode = LR35902OperandMode::NZ;
    else if (op == "Z")
        result.mode = LR35902OperandMode::Z;
    else if (op == "NC")
        result.mode = LR35902OperandMode::NC;

    // Register-indirect memory forms
    else if (op == "(BC)")
        result.mode = LR35902OperandMode::AddrBC;
    else if (op == "(DE)")
        result.mode = LR35902OperandMode::AddrDE;
    else if (op == "(HL)")
        result.mode = LR35902OperandMode::AddrHL;
    else if (op == "(HL+)" || op == "(HLI)")
        result.mode = LR35902OperandMode::AddrHLInc;
    else if (op == "(HL-)" || op == "(HLD)")
        result.mode = LR35902OperandMode::AddrHLDec;
    else if (op == "(C)" || op == "($FF00+C)")
        result.mode = LR35902OperandMode::AddrC;

    // SP plus signed immediate
    else if (op.rfind("SP+", 0) == 0 ||
             op.rfind("SP-", 0) == 0)
    {
        const char sign = op[2];

        const uint16_t magnitude =
            parseNumber(op.substr(3));

        if (magnitude > 128)
        {
            throw std::runtime_error(
                "SP-relative operand is out of range: " +
                operand);
        }

        int value = static_cast<int>(magnitude);

        if (sign == '-')
            value = -value;

        if (value < -128 || value > 127)
        {
            throw std::runtime_error(
                "SP-relative operand is out of range: " +
                operand);
        }

        result.mode =
            LR35902OperandMode::SPPlusSignedImm8;

        result.value =
            static_cast<uint8_t>(
                static_cast<int8_t>(value));

        result.hasValue = true;
    }

    // Memory through an immediate address
    else if (op.front() == '(' && op.back() == ')')
    {
        const std::string inner =
            op.substr(1, op.size() - 2);

        const uint16_t value =
            parseNumber(inner);

        result.value = value;
        result.hasValue = true;

        // E0/F0 use an 8-bit offset from $FF00.
        if (value >= 0xFF00)
        {
            result.mode =
                LR35902OperandMode::AddrImm8;

            result.value =
                static_cast<uint8_t>(
                    value - 0xFF00);
        }
        else
        {
            result.mode =
                LR35902OperandMode::AddrImm16;
        }
    }

    // RST vectors
    else if (mnemonic == "RST")
    {
        const uint16_t value = parseNumber(op);

        result.value = value;
        result.hasValue = true;

        switch (value)
        {
            case 0x00:
                result.mode = LR35902OperandMode::Rst00;
                break;

            case 0x08:
                result.mode = LR35902OperandMode::Rst08;
                break;

            case 0x10:
                result.mode = LR35902OperandMode::Rst10;
                break;

            case 0x18:
                result.mode = LR35902OperandMode::Rst18;
                break;

            case 0x20:
                result.mode = LR35902OperandMode::Rst20;
                break;

            case 0x28:
                result.mode = LR35902OperandMode::Rst28;
                break;

            case 0x30:
                result.mode = LR35902OperandMode::Rst30;
                break;

            case 0x38:
                result.mode = LR35902OperandMode::Rst38;
                break;

            default:
                throw std::runtime_error(
                    "Invalid RST vector: " + operand);
        }
    }

    // BIT, RES and SET bit-number operand
    else if ((mnemonic == "BIT" ||
              mnemonic == "RES" ||
              mnemonic == "SET") &&
             operandIndex == 0)
    {
        const uint16_t bit = parseNumber(op);

        if (bit > 7)
        {
            throw std::runtime_error(
                "Bit number must be between 0 and 7.");
        }

        result.mode =
            static_cast<LR35902OperandMode>(
                static_cast<int>(
                    LR35902OperandMode::Bit0) +
                static_cast<int>(bit));

        result.value = bit;
        result.hasValue = true;
    }

    // Signed immediate used by ADD SP,e8
    else if (mnemonic == "ADD" &&
             operandIndex == 1 &&
             (op.front() == '+' ||
              op.front() == '-'))
    {
        const bool negative =
            op.front() == '-';

        const uint16_t magnitude =
            parseNumber(op.substr(1));

        int value =
            static_cast<int>(magnitude);

        if (negative)
            value = -value;

        if (value < -128 || value > 127)
        {
            throw std::runtime_error(
                "Signed immediate is out of range: " +
                operand);
        }

        result.mode =
            LR35902OperandMode::SignedImm8;

        result.value =
            static_cast<uint8_t>(
                static_cast<int8_t>(value));

        result.hasValue = true;
    }

    // Numeric operands
    else
    {
        const uint16_t value = parseNumber(op);

        result.value = value;
        result.hasValue = true;

        if (mnemonic == "JR")
        {
            // Numeric JR operands are treated as target addresses.
            result.mode = LR35902OperandMode::Rel8;
        }
        else if (value <= 0xFF)
        {
            result.mode = LR35902OperandMode::Imm8;
        }
        else
        {
            result.mode = LR35902OperandMode::Imm16;
        }
    }

    return result;
}

const LR35902InstructionInfo&
LR35902Assembler::lookupInstruction(
    const std::string& mnemonic,
    const ParsedOperand& operand1,
    const ParsedOperand& operand2) const
{
    auto tryLookup =
        [&](LR35902OperandMode mode1,
            LR35902OperandMode mode2)
            -> const LR35902InstructionInfo*
    {
        const LR35902MnemonicKey key
        {
            mnemonic,
            mode1,
            mode2
        };

        const auto iterator =
            LR35902_MNEMONIC_TO_OPCODE.find(key);

        if (iterator == LR35902_MNEMONIC_TO_OPCODE.end())
            return nullptr;

        return &iterator->second;
    };

    // First try the exact operand modes produced by the parser.
    if (const LR35902InstructionInfo* info =
            tryLookup(
                operand1.mode,
                operand2.mode))
    {
        return *info;
    }

    /*
     * A numeric value such as $0040 fits in eight bits, but JP and
     * CALL still require a 16-bit immediate operand.
     */
    if (operand1.hasValue &&
        operand1.mode == LR35902OperandMode::Imm8)
    {
        if (const LR35902InstructionInfo* info =
                tryLookup(
                    LR35902OperandMode::Imm16,
                    operand2.mode))
        {
            return *info;
        }
    }

    if (operand2.hasValue &&
        operand2.mode == LR35902OperandMode::Imm8)
    {
        if (const LR35902InstructionInfo* info =
                tryLookup(
                    operand1.mode,
                    LR35902OperandMode::Imm16))
        {
            return *info;
        }
    }

    /*
     * Numeric JR operands represent destination addresses and must
     * use the signed relative operand mode.
     */
    if (mnemonic == "JR")
    {
        if (operand1.hasValue &&
            operand2.mode == LR35902OperandMode::None)
        {
            if (const LR35902InstructionInfo* info =
                    tryLookup(
                        LR35902OperandMode::Rel8,
                        LR35902OperandMode::None))
            {
                return *info;
            }
        }

        if (operand2.hasValue)
        {
            if (const LR35902InstructionInfo* info =
                    tryLookup(
                        operand1.mode,
                        LR35902OperandMode::Rel8))
            {
                return *info;
            }
        }
    }

    throw std::runtime_error(
        "Unsupported LR35902 instruction: " +
        mnemonic +
        " (" +
        std::to_string(
            static_cast<int>(operand1.mode)) +
        ", " +
        std::to_string(
            static_cast<int>(operand2.mode)) +
        ")");
}

std::string LR35902Assembler::trim(
    const std::string& text)
{
    const auto first =
        std::find_if_not(
            text.begin(),
            text.end(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            });

    if (first == text.end())
        return "";

    const auto last =
        std::find_if_not(
            text.rbegin(),
            text.rend(),
            [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            }).base();

    return std::string(first, last);
}

std::string LR35902Assembler::toUpper(
    std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(
                std::toupper(ch));
        });

    return text;
}

std::vector<std::string>
LR35902Assembler::splitOperands(
    const std::string& operandText)
{
    std::vector<std::string> operands;

    const std::string text = trim(operandText);

    if (text.empty())
        return operands;

    int parenthesisDepth = 0;
    std::size_t start = 0;

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '(')
        {
            ++parenthesisDepth;
        }
        else if (text[i] == ')')
        {
            --parenthesisDepth;

            if (parenthesisDepth < 0)
            {
                throw std::runtime_error(
                    "Unmatched closing parenthesis.");
            }
        }
        else if (text[i] == ',' &&
                 parenthesisDepth == 0)
        {
            operands.push_back(
                trim(text.substr(start, i - start)));

            start = i + 1;
        }
    }

    if (parenthesisDepth != 0)
    {
        throw std::runtime_error(
            "Unmatched opening parenthesis.");
    }

    operands.push_back(
        trim(text.substr(start)));

    return operands;
}

uint16_t LR35902Assembler::parseNumber(
    const std::string& text)
{
    std::string valueText = trim(text);

    if (valueText.empty())
        throw std::runtime_error("Missing numeric value.");

    int base = 16;

    if (valueText.front() == '$')
    {
        valueText.erase(valueText.begin());
    }
    else if (valueText.size() >= 2 &&
             valueText[0] == '0' &&
             (valueText[1] == 'X' ||
              valueText[1] == 'x'))
    {
        valueText = valueText.substr(2);
    }

    if (valueText.empty())
        throw std::runtime_error("Missing numeric value.");

    std::size_t consumed = 0;

    const unsigned long value =
        std::stoul(
            valueText,
            &consumed,
            base);

    if (consumed != valueText.size())
    {
        throw std::runtime_error(
            "Invalid numeric value: " + text);
    }

    if (value > 0xFFFF)
    {
        throw std::runtime_error(
            "Numeric value is larger than 16 bits: " +
            text);
    }

    return static_cast<uint16_t>(value);
}

void LR35902Assembler::appendImmediateBytes(
    std::vector<uint8_t>& bytes,
    const LR35902InstructionInfo& info,
    const ParsedOperand& operand1,
    const ParsedOperand& operand2,
    uint16_t instructionAddress)
{
    const ParsedOperand* valueOperand = nullptr;

    if (operand1.hasValue)
        valueOperand = &operand1;

    if (operand2.hasValue)
        valueOperand = &operand2;

    if (info.length <= bytes.size())
        return;

    if (valueOperand == nullptr)
    {
        throw std::runtime_error(
            "Instruction requires an immediate operand.");
    }

    uint16_t value = valueOperand->value;

    if (valueOperand->mode == LR35902OperandMode::Rel8)
    {
        const uint16_t nextAddress =
            static_cast<uint16_t>(
                instructionAddress + info.length);

        const int32_t displacement =
            static_cast<int32_t>(value) -
            static_cast<int32_t>(nextAddress);

        if (displacement < -128 ||
            displacement > 127)
        {
            throw std::runtime_error(
                "JR target is outside the signed "
                "8-bit relative range.");
        }

        value =
            static_cast<uint8_t>(
                static_cast<int8_t>(
                    displacement));
    }

    if (info.length > bytes.size())
    {
        bytes.push_back(
            static_cast<uint8_t>(
                value & 0x00FF));
    }

    if (info.length > bytes.size())
    {
        bytes.push_back(
            static_cast<uint8_t>(
                (value >> 8) & 0x00FF));
    }
}
