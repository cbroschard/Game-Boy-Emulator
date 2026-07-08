// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "debug/CPUCommand.h"
#include "debug/MLMonitor.h"
#include "debug/MLMonitorBackend.h"

CPUCommand::CPUCommand()= default;

CPUCommand::~CPUCommand() = default;

int CPUCommand::order() const
{
    return 1;
}

std::string CPUCommand::category() const
{
    return "CPU/Execution";
}

std::string CPUCommand::name() const
{
    return "cpu";
}

std::string CPUCommand::shortHelp() const
{
    return "cpu       - CPU state and debugging (regs, flags, irq, cycles, stack)";
}

std::string CPUCommand::help() const
{
    return
        "cpu [regs|flags|irq|cycles|stack] [count]\n"
        "    Show Game Boy LR35902 CPU state and debugging information.\n"
        "\n"
        "Arguments:\n"
        "    regs       Show CPU registers, flags, interrupt state, and cycle count.\n"
        "    flags      Show decoded Game Boy CPU flag bits from the F register.\n"
        "               Flags are Z, N, H, and C.\n"
        "    irq        Show interrupt state such as IME, pending EI state,\n"
        "               HALT state, STOP state, and pending interrupt information if available.\n"
        "    cycles     Show the total CPU cycle count.\n"
        "    stack      Dump words from memory starting at the current stack pointer.\n"
        "    [count]    Optional number of stack words to show.\n"
        "               Defaults to 8 if not provided.\n"
        "\n"
        "Examples:\n"
        "    cpu             Show CPU registers and main state.\n"
        "    cpu help        Show this help.\n"
        "    cpu regs        Show CPU registers and flags.\n"
        "    cpu flags       Show decoded Game Boy CPU flags.\n"
        "    cpu irq         Show interrupt state.\n"
        "    cpu cycles      Show CPU cycle count.\n"
        "    cpu stack       Show 8 words from the current stack pointer.\n"
        "    cpu stack 16    Show 16 words from the current stack pointer.\n"
        "\n";
}

void CPUCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    if (args.size() == 1)
    {
        std::cout << "Usage:\n" << help();
        return;
    }

    auto backend = mlMonitor.getMLMonitorBackend();
    if (!backend)
    {
        std::cerr << "Error: Monitor backend not available\n";
        return;
    }

    const std::string& sub = args[1];

    if (isHelp(sub))
    {
        std::cout << "Usage:\n" << help() << std::endl;
        return;
    }

    else if (sub == "regs")
    {
        backend->printCPUState();
        return;
    }

    else if (sub == "flags")
    {
        backend->printCPUFlags();
        return;
    }

    else if (sub == "irq")
    {
        backend->printCPUIRQState();
        return;
    }

    else if (sub == "cycles")
    {
        backend->printCPUCycles();
        return;
    }

    else if (args[1] == "stack")
    {
        int count = 8;

        if (args.size() >= 3)
        {
            try
            {
                count = std::stoi(args[2]);
            }
            catch (...)
            {
                std::cout << "Invalid stack count." << std::endl;
                return;
            }

            if (count <= 0)
            {
                std::cout << "Invalid stack count." << std::endl;
                return;
            }
        }

        backend->printCPUStack(count);
        return;
    }

    else
    {
        std::cout << "Usage:\n" << help() << std::endl;
        return;
    }
}
