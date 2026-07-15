// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <iomanip>
#include <iostream>
#include <sstream>
#include "Debug/MLMonitorBackend.h"
#include "APU.h"
#include "Cartridge.h"
#include "EmulationSession.h"
#include "InputManager.h"
#include "Memory.h"
#include "PPU.h"
#include "Timer.h"

MLMonitorBackend::MLMonitorBackend() :
    apu(nullptr),
    bus(nullptr),
    cartridge(nullptr),
    cpu(nullptr),
    host(nullptr),
    inputManager(nullptr),
    memory(nullptr),
    ppu(nullptr),
    timer(nullptr)
{

}

MLMonitorBackend::~MLMonitorBackend()
{

}

void MLMonitorBackend::enterMonitor()
{
    if (host)
        host->enterMonitor();
}

Cartridge::CartridgeInfo
MLMonitorBackend::getCartridgeInfo() const
{
    if (cartridge == nullptr)
        return Cartridge::CartridgeInfo{};

    return cartridge->getCartridgeInfo();
}

void MLMonitorBackend::printCartridgeInfo() const
{
    if (cartridge == nullptr)
    {
        std::cout
            << "Cartridge is not attached.\n";
        return;
    }

    const Cartridge::CartridgeInfo info =
        cartridge->getCartridgeInfo();

    if (!info.loaded)
    {
        std::cout
            << "No cartridge is currently loaded.\n";
        return;
    }

    auto formatSize =
        [](std::size_t bytes) -> std::string
        {
            std::ostringstream output;

            if (bytes >= 1024 * 1024)
            {
                output
                    << (bytes / (1024 * 1024))
                    << " MB";
            }
            else if (bytes >= 1024)
            {
                output
                    << (bytes / 1024)
                    << " KB";
            }
            else
            {
                output
                    << bytes
                    << " bytes";
            }

            return output.str();
        };

    std::cout
        << "Cartridge Information\n"
        << "---------------------\n";

    std::cout
        << "Title:          "
        << (info.title.empty()
                ? "<untitled>"
                : info.title)
        << "\n";

    std::cout
        << "Type:           "
        << info.cartridgeType
        << " ($"
        << std::hex
        << std::uppercase
        << std::setw(2)
        << std::setfill('0')
        << static_cast<unsigned int>(
            info.cartridgeTypeCode)
        << ")\n";

    std::cout
        << "Mapper:         "
        << info.mapperType
        << "\n";

    std::cout
        << std::dec
        << std::nouppercase
        << std::setfill(' ');

    std::cout
        << "ROM size:       "
        << formatSize(info.romSizeBytes)
        << "\n";

    std::cout
        << "ROM banks:      "
        << info.romBankCount
        << "\n";

    std::cout
        << "RAM size:       "
        << formatSize(info.ramSizeBytes)
        << "\n";

    std::cout
        << "RAM banks:      "
        << info.ramBankCount
        << "\n";

    std::cout
        << "Current ROM:    "
        << info.selectedROMBank
        << "\n";

    std::cout
        << "Current RAM:    "
        << static_cast<unsigned int>(
            info.selectedRAMBank)
        << "\n";

    std::cout
        << "Banking mode:   "
        << static_cast<unsigned int>(
            info.bankingMode)
        << "\n";

    std::cout
        << "RAM present:    "
        << (info.hasRAM ? "Yes" : "No")
        << "\n";

    std::cout
        << "RAM enabled:    "
        << (info.ramEnabled ? "Yes" : "No")
        << "\n";

    std::cout
        << "Battery:        "
        << (info.hasBattery ? "Yes" : "No")
        << "\n";

    std::cout
        << "Timer:          "
        << (info.hasTimer ? "Yes" : "No")
        << "\n";

    std::cout
        << "Rumble:         "
        << (info.hasRumble ? "Yes" : "No")
        << "\n";

    std::cout
        << "SGB support:    "
        << (info.sgbFlag == 0x03
                ? "Yes"
                : "No")
        << "\n";

    std::cout
        << "Destination:    "
        << (info.destinationCode == 0x00
                ? "Japan"
                : "Non-Japan")
        << "\n";

    std::cout
        << "ROM version:    "
        << static_cast<unsigned int>(
            info.maskROMVersion)
        << "\n";

    std::cout
        << "Header checksum: $"
        << std::hex
        << std::uppercase
        << std::setw(2)
        << std::setfill('0')
        << static_cast<unsigned int>(
            info.headerChecksum)
        << "\n";

    std::cout
        << std::dec
        << std::nouppercase
        << std::setfill(' ');
}

void MLMonitorBackend::printCPUCycles() const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const lr35902CPUState state = cpu->getCPUState();

    std::cout
        << std::dec
        << "CPU Cycles:\n"
        << "  Total:        " << state.cycles << "\n";
}

void MLMonitorBackend::printCPUFlags() const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const lr35902CPUState state = cpu->getCPUState();

    std::cout << "CPU Flags" << std::endl;
    std::cout << "---------" << std::endl;

    std::cout << "Z: " << (state.flagZ ? "1" : "0")
              << "  N: " << (state.flagN ? "1" : "0")
              << "  H: " << (state.flagH ? "1" : "0")
              << "  C: " << (state.flagC ? "1" : "0")
              << std::endl;

    std::cout << "F: $"
              << std::hex << std::uppercase
              << std::setw(2) << std::setfill('0')
              << static_cast<int>(state.F)
              << std::dec << std::nouppercase
              << std::setfill(' ')
              << std::endl;
}

void MLMonitorBackend::printCPUIRQState() const
{
    if (!cpu)

    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const lr35902CPUState state = cpu->getCPUState();

    std::cout << "CPU Interrupt State" << std::endl;
    std::cout << "-------------------" << std::endl;

    std::cout << "IME:        " << (state.IME ? "1" : "0") << std::endl;
    std::cout << "EI Pending: " << (state.imeEnablePending ? "1" : "0") << std::endl;
    std::cout << "Halted:     " << (state.halted ? "1" : "0") << std::endl;
    std::cout << "Stopped:    " << (state.stopped ? "1" : "0") << std::endl;
}

void MLMonitorBackend::printCPUStack(int count) const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    if (!bus)
    {
        std::cout << "Bus is not attached." << std::endl;
        return;
    }

    if (count <= 0)
        count = 8;

    const lr35902CPUState state = cpu->getCPUState();

    std::cout << "CPU Stack" << std::endl;
    std::cout << "---------" << std::endl;

    std::cout << std::hex << std::uppercase << std::setfill('0');

    std::cout << "SP: $" << std::setw(4) << state.SP << std::endl;
    std::cout << std::endl;

    for (int i = 0; i < count; ++i)
    {
        uint16_t address = uint16_t(state.SP + (i * 2));

        uint8_t lo = bus->read(address);
        uint8_t hi = bus->read(uint16_t(address + 1));

        uint16_t value = uint16_t(lo) | (uint16_t(hi) << 8);

        std::cout << "$" << std::setw(4) << address
                  << ": $"
                  << std::setw(4) << value;

        if (i == 0)
            std::cout << "  <- SP";

        std::cout << std::endl;
    }

    std::cout << std::dec << std::nouppercase << std::setfill(' ');
}

void MLMonitorBackend::printCPUState() const
{
    if (!cpu)
    {
        std::cout << "CPU is not attached." << std::endl;
        return;
    }

    const lr35902CPUState state = cpu->getCPUState();

    std::cout << std::hex << std::uppercase << std::setfill('0');

    std::cout << "CPU Registers" << std::endl;
    std::cout << "-------------" << std::endl;

    std::cout << "PC: $" << std::setw(4) << state.PC
              << "   SP: $" << std::setw(4) << state.SP
              << std::endl << std::endl;

    std::cout << "AF: $" << std::setw(4) << state.AF
              << "   BC: $" << std::setw(4) << state.BC
              << "   DE: $" << std::setw(4) << state.DE
              << "   HL: $" << std::setw(4) << state.HL
              << std::endl;

    std::cout << "A:  $" << std::setw(2) << static_cast<int>(state.A)
              << "     F:  $" << std::setw(2) << static_cast<int>(state.F)
              << std::endl;

    std::cout << "B:  $" << std::setw(2) << static_cast<int>(state.B)
              << "     C:  $" << std::setw(2) << static_cast<int>(state.C)
              << std::endl;

    std::cout << "D:  $" << std::setw(2) << static_cast<int>(state.D)
              << "     E:  $" << std::setw(2) << static_cast<int>(state.E)
              << std::endl;

    std::cout << "H:  $" << std::setw(2) << static_cast<int>(state.H)
              << "     L:  $" << std::setw(2) << static_cast<int>(state.L)
              << std::endl << std::endl;

    std::cout << "Flags: "
              << "Z=" << (state.flagZ ? "1" : "0") << " "
              << "N=" << (state.flagN ? "1" : "0") << " "
              << "H=" << (state.flagH ? "1" : "0") << " "
              << "C=" << (state.flagC ? "1" : "0")
              << std::endl << std::endl;

    std::cout << "IME: " << (state.IME ? "1" : "0")
              << "   EI Pending: " << (state.imeEnablePending ? "1" : "0")
              << std::endl;

    std::cout << "Halted: " << (state.halted ? "1" : "0")
              << "   Stopped: " << (state.stopped ? "1" : "0")
              << std::endl << std::endl;

    std::cout << std::dec;
    std::cout << "Cycles: " << state.cycles << std::endl;

    std::cout << std::setfill(' ') << std::nouppercase;
}

LR35902DisassembledInstruction MLMonitorBackend::disassembleAt(uint16_t address) const
{
    LR35902Disassembler disassembler(
        [this](uint16_t address) -> uint8_t
        {
            return debugRead(address);
        });

    return disassembler.disassemble(address);
}

uint8_t MLMonitorBackend::debugRead(uint16_t address) const
{
    return bus->read(address);
}

int MLMonitorBackend::stepInstruction()
{
    if (cpu == nullptr)
        return 0;

    return cpu->debugStepInstruction();
}
