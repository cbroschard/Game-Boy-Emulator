// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "debug/DisassembleCommand.h"
#include "debug/lr35902/LR35902Disassembler.h"
#include "debug/MLMonitor.h"

DisassembleCommand::DisassembleCommand() :
    currentPC(0x0000),
    currentPCInitialized(false)
{

}

DisassembleCommand::~DisassembleCommand() = default;

int DisassembleCommand::order() const
{
    return 10;
}

std::string DisassembleCommand::name() const
{
    return "d";
}

std::string DisassembleCommand::category() const
{
    return "Debugging";
}

std::string DisassembleCommand::shortHelp() const
{
    return "d <addr> [count] - Disassemble instructions starting at address";
}

std::string DisassembleCommand::help() const
{
return
"d [address|pc] [count]\n"
"    Disassemble memory into Game Boy LR35902 assembly instructions\n"
"    starting at the specified address.\n"
"\n"
"Arguments:\n"
"    [address]   Optional 16-bit starting address.\n"
"                Hex prefixes $ and 0x are optional.\n"
"                Examples: $0100, 0x4000, C000, FF80.\n"
"    pc          Start at the current LR35902 program counter.\n"
"    [count]     Optional number of instructions to disassemble.\n"
"                Defaults to 16 if not provided.\n"
"\n"
"Common Game Boy locations:\n"
"    $0000       Restart vector / ROM bank 0.\n"
"    $0040       VBlank interrupt vector.\n"
"    $0048       LCD STAT interrupt vector.\n"
"    $0050       Timer interrupt vector.\n"
"    $0058       Serial interrupt vector.\n"
"    $0060       Joypad interrupt vector.\n"
"    $0100       Cartridge entry point.\n"
"    $4000       Switchable cartridge ROM bank.\n"
"    $C000       Work RAM.\n"
"    $FF80       High RAM.\n"
"\n"
"Examples:\n"
"    d             Continue from the previous disassembly address.\n"
"    d help        Show this help.\n"
"    d pc          Disassemble 16 instructions from the current PC.\n"
"    d pc 32       Disassemble 32 instructions from the current PC.\n"
"    d $0100       Disassemble the cartridge entry point.\n"
"    d $0040 8     Disassemble the VBlank interrupt handler.\n"
"    d $4000 20    Disassemble 20 instructions from ROM bank space.\n"
"\n";
}

void DisassembleCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    MLMonitorBackend* backend = mlMonitor.getMLMonitorBackend();

    if (backend == nullptr)
    {
        std::cout << "Monitor backend is not attached." << std::endl;
        return;
    }

    CPU& cpu = backend->getCPU();

    uint16_t address = 0;
    int count = 16;

    if (!currentPCInitialized)
    {
        currentPC = cpu.getPC();
        currentPCInitialized = true;
    }

    // d
    if (args.size() < 2)
    {
        address = currentPC;
    }
    // d help / d ?
    else if (isHelp(args[1]))
    {
        std::cout << "Usage:\n" << help() << std::endl;
        return;
    }
    // d pc / d pc 20
    else if (args[1] == "pc")
    {
        address = cpu.getPC();
        currentPC = address;

        if (args.size() >= 3)
            count = static_cast<int>(parseAddress(args[2]));
    }
    // d $8000 / d $8000 20
    else
    {
        address = parseAddress(args[1]);
        currentPC = address;

        if (args.size() >= 3)
            count = static_cast<int>(parseAddress(args[2]));
    }

    LR35902Disassembler disassembler(
        [backend](uint16_t addr) -> uint8_t
        {
            return backend->readRAM(addr);
        });

    for (int i = 0; i < count; ++i)
    {
        LR35902DisassembledInstruction inst = disassembler.disassemble(address);

        std::cout
            << "$" << hex4(inst.address)
            << "  "
            << formatBytes(inst.bytes, inst.size)
            << "  "
            << inst.text
            << std::endl;

        address = static_cast<uint16_t>(address + inst.size);
    }

    currentPC = address;
}

std::string DisassembleCommand::formatBytes(const uint8_t* bytes, int size) const
{
    std::ostringstream ss;

    for (int i = 0; i < size; ++i)
    {
        if (i > 0)
            ss << " ";

        ss << hex2(bytes[i]);
    }

    return ss.str();
}
