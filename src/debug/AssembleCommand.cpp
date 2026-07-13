// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.

#include "debug/AssembleCommand.h"
#include "debug/lr35902/LR35902Assembler.h"
#include "debug/MLMonitor.h"

#include <iomanip>
#include <iostream>
#include <sstream>

AssembleCommand::AssembleCommand() :
    interactiveActive(false),
    interactiveAddress(0x0000)
{
}

AssembleCommand::~AssembleCommand() = default;

int AssembleCommand::order() const
{
    return 3;
}

std::string AssembleCommand::name() const
{
    return "a";
}

std::string AssembleCommand::category() const
{
    return "Debugging";
}

std::string AssembleCommand::shortHelp() const
{
    return "a <addr> [instruction] - Assemble LR35902 instruction(s) into memory";
}

std::string AssembleCommand::help() const
{
    return R"(a - Assemble Game Boy LR35902 instructions into memory

Usage:
    a <address>
    a <address> <mnemonic> [operand1[, operand2]]

Modes:
    a <address>
        Starts interactive assembly mode at the specified address.

        Enter one LR35902 instruction per line. The address advances
        automatically by the encoded instruction length.

        Press Enter on a blank line, or enter '.', to end assembly.

    a <address> <instruction>
        Assembles one LR35902 instruction and writes its bytes directly
        into Game Boy memory.

Address formats:
    Hexadecimal prefixes $ and 0x are optional.

    Examples:
        $C000
        0xC000
        C000

Common Game Boy locations:
    $0000       ROM bank 0 / RST $00 vector
    $0040       VBlank interrupt vector
    $0048       LCD STAT interrupt vector
    $0050       Timer interrupt vector
    $0058       Serial interrupt vector
    $0060       Joypad interrupt vector
    $0100       Cartridge entry point
    $4000       Switchable cartridge ROM bank
    $8000       Video RAM
    $C000       Work RAM
    $FF80       High RAM

Examples:
    a $C000
    a $C000 LD A,$01
    a $C002 LD B,$20
    a $C004 LD ($C100),A
    a $C007 INC B
    a $C008 ADD A,C
    a $C009 BIT 7,A
    a $C00B SET 0,(HL)
    a $C00D JR $C020
    a $C00F JR NZ,$C020
    a $C011 JP $0150
    a $C014 CALL $0200
    a $C017 LD HL,SP+$08
    a $C019 ADD SP,-$04
    a $C01B RST $38
    a $FF80 NOP

Operand syntax:
    8-bit registers:
        A, B, C, D, E, H, L

    16-bit registers:
        AF, BC, DE, HL, SP

    Register-indirect memory:
        (BC), (DE), (HL), (HL+), (HL-), (C)

    Immediate values:
        $12
        $1234
        0x12
        0x1234

    Immediate memory addresses:
        ($C000)
        ($FF80)

    Conditions:
        NZ, Z, NC, C

    Relative branches:
        JR $C020
        JR NZ,$C020

        JR operands are destination addresses. The assembler calculates
        the signed 8-bit displacement automatically.

    Bit operations:
        BIT 0,A
        RES 4,(HL)
        SET 7,C

Notes:
    - Do not use the 6502 immediate prefix '#'.
      Use LD A,$12 instead of LDA #$12.

    - CB-prefixed instructions such as BIT, RES, SET, RLC, and SWAP
      automatically receive the $CB prefix.

    - Memory is updated immediately after each instruction.

    - Cartridge ROM is normally read-only or mapper-controlled. Work RAM
      at $C000 and High RAM at $FF80 are better locations for test code.

    - Use the 'm' command to inspect the encoded bytes.

    - Use the 'd' command to disassemble and verify the instructions.
)";
}

bool AssembleCommand::isInteractiveActive() const
{
    return interactiveActive;
}

bool AssembleCommand::assembleAndWrite(
    MLMonitor& mlMonitor,
    uint16_t address,
    const std::string& line,
    uint16_t& nextAddress)
{
    MLMonitorBackend* backend =
        mlMonitor.getMLMonitorBackend();

    if (backend == nullptr)
    {
        std::cout << "Monitor backend is not attached.\n";
        return false;
    }

    LR35902Assembler assembler;

    const LR35902Assembler::AssembledInstruction instruction =
        assembler.assembleLine(line, address);

    for (std::size_t i = 0;
         i < instruction.bytes.size();
         ++i)
    {
        backend->writeRAM(
            static_cast<uint16_t>(address + i),
            instruction.bytes[i]);
    }

    std::cout
        << "$"
        << std::uppercase
        << std::hex
        << std::setw(4)
        << std::setfill('0')
        << static_cast<unsigned int>(address)
        << "  ";

    for (uint8_t byte : instruction.bytes)
    {
        std::cout
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned int>(byte)
            << " ";
    }

    std::cout
        << "  "
        << line
        << "\n"
        << std::dec
        << std::nouppercase
        << std::setfill(' ');

    nextAddress = instruction.nextAddress;

    return true;
}

bool AssembleCommand::handleInteractiveLine(
    MLMonitor& mlMonitor,
    const std::string& line)
{
    if (!interactiveActive)
        return false;

    const std::string trimmed = trimCopy(line);

    if (trimmed.empty() || trimmed == ".")
    {
        interactiveActive = false;
        std::cout << "Assembly ended.\n";
        return true;
    }

    try
    {
        uint16_t nextAddress = interactiveAddress;

        if (assembleAndWrite(
                mlMonitor,
                interactiveAddress,
                trimmed,
                nextAddress))
        {
            interactiveAddress = nextAddress;
        }
    }
    catch (const std::exception& exception)
    {
        std::cout
            << "Assembly error: "
            << exception.what()
            << "\n";
    }

    return true;
}

std::string AssembleCommand::currentPrompt() const
{
    std::ostringstream out;

    out << "$"
        << std::uppercase
        << std::hex
        << std::setw(4)
        << std::setfill('0')
        << static_cast<unsigned int>(interactiveAddress)
        << " > ";

    return out.str();
}

void AssembleCommand::execute(
    MLMonitor& mlMonitor,
    const std::vector<std::string>& args)
{
    if (args.size() == 1 ||
        (args.size() > 1 && isHelp(args[1])))
    {
        std::cout << help() << std::endl;
        return;
    }

    uint16_t address = 0;

    try
    {
        address = parseAddress(args[1]);
    }
    catch (const std::exception&)
    {
        std::cout
            << "Invalid address: "
            << args[1]
            << "\n"
            << "Usage: a <address> [instruction]\n";

        return;
    }

    MLMonitorBackend* backend =
        mlMonitor.getMLMonitorBackend();

    if (backend == nullptr)
    {
        std::cout << "Monitor backend is not attached.\n";
        return;
    }

    // Interactive mode:
    // a $C000
    if (args.size() == 2)
    {
        interactiveActive = true;
        interactiveAddress = address;

        std::cout
            << "LR35902 assembly started at $"
            << std::uppercase
            << std::hex
            << std::setw(4)
            << std::setfill('0')
            << static_cast<unsigned int>(interactiveAddress)
            << std::dec
            << std::nouppercase
            << std::setfill(' ')
            << ". Enter a blank line or '.' to finish.\n";

        return;
    }

    // One-line mode:
    // a $C000 LD A,$01
    const std::string line =
        joinArgs(args, 2);

    try
    {
        uint16_t nextAddress = address;

        assembleAndWrite(
            mlMonitor,
            address,
            line,
            nextAddress);
    }
    catch (const std::exception& exception)
    {
        std::cout
            << "Assembly error: "
            << exception.what()
            << "\n";
    }
}
