// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <iostream>
#include "debug/CartridgeCommand.h"
#include "debug/MLMonitor.h"

CartridgeCommand::CartridgeCommand() = default;

CartridgeCommand::~CartridgeCommand() = default;

int CartridgeCommand::order() const
{
    return 30;
}

std::string CartridgeCommand::name() const
{
    return "cart";
}

std::string CartridgeCommand::category() const
{
    return "Cartridge";
}

std::string CartridgeCommand::shortHelp() const
{
    return "cart      - Display loaded cartridge information";
}

std::string CartridgeCommand::help() const
{
    return
        "cart\n"
        "\n"
        "Displays information about the currently loaded Game Boy cartridge,\n"
        "including its title, mapper, ROM and RAM sizes, selected banks, and\n"
        "hardware features.\n";
}

void CartridgeCommand::execute(
    MLMonitor& mlMonitor,
    const std::vector<std::string>& args)
{
    if (args.size() > 1 &&
        isHelp(args[1]))
    {
        std::cout << help();
        return;
    }

    MLMonitorBackend* backend =
        mlMonitor.getMLMonitorBackend();

    if (backend == nullptr)
    {
        std::cout
            << "Monitor backend is not attached.\n";
        return;
    }

    backend->printCartridgeInfo();
}
