// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "debug/MLMonitor.h"
#include "debug/StepCommand.h"

StepCommand::StepCommand() = default;

StepCommand::~StepCommand() = default;

int StepCommand::order() const
{
    return 5;
}

std::string StepCommand::name() const
{
    return "t";
}

std::string StepCommand::category() const
{
    return "CPU/Execution";
}

std::string StepCommand::shortHelp() const
{
    return "t         - Step one CPU instruction";
}

std::string StepCommand::help() const
{
    return
        "t    Execute exactly one CPU instruction and then return to the monitor.\n"
        "     After stepping, registers are shown automatically.\n";
}

void StepCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    if (args.size() > 1 && isHelp(args[1]))
    {
        std::cout << help() << std::endl;
        return;
    }

    MLMonitorBackend* backend = mlMonitor.getMLMonitorBackend();

    if (backend == nullptr)
    {
        std::cout << "Monitor backend is not attached." << std::endl;
        return;
    }

    // Get current PC before stepping
    const uint16_t pc = backend->getPC();

    // Disassemble instruction at current PC
    const LR35902DisassembledInstruction instruction = backend->disassembleAt(pc);

    // Print instruction
    std::cout << "$"
              << std::uppercase
              << std::hex
              << std::setw(4)
              << std::setfill('0')
              << instruction.address
              << "  ";

    for (uint8_t i = 0; i < instruction.size; ++i)
    {
        std::cout << std::setw(2)
                  << static_cast<int>(instruction.bytes[i])
                  << " ";
    }

    for (uint8_t i = instruction.size; i < 4; ++i)
        std::cout << "   ";

    std::cout << " " << instruction.text << std::dec << std::endl;

    // Execute exactly one Z80 instruction
    const int cyclesUsed = backend->stepInstruction();

    // Show updated CPU state
    backend->printCPUState();

    std::cout << "Step cycles: " << cyclesUsed << std::endl;
}
