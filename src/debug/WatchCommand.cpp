// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "debug/MLMonitor.h"
#include "debug/WatchCommand.h"

namespace
{
    struct WatchRange
    {
        uint16_t start = 0;
        uint16_t end = 0;
    };

    WatchRange parseAddressOrRange(const std::string& s)
    {
        const auto dash = s.find('-');
        if (dash == std::string::npos)
        {
            uint16_t a = parseAddress(s);
            return { a, a };
        }

        const std::string lhs = s.substr(0, dash);
        const std::string rhs = s.substr(dash + 1);

        uint16_t start = parseAddress(lhs);
        uint16_t end   = parseAddress(rhs);

        if (end < start)
            std::swap(start, end);

        return { start, end };
    }

    void applyAddRange(MLMonitor& mlMonitor, uint8_t bits, uint16_t start, uint16_t end)
    {
        for (uint32_t a = start; a <= end; ++a)
        {
            if (bits & 1) mlMonitor.addReadWatch(static_cast<uint16_t>(a));
            if (bits & 2) mlMonitor.addWriteWatch(static_cast<uint16_t>(a));
        }
    }

    void applyClearRange(MLMonitor& mlMonitor, uint8_t bits, uint16_t start, uint16_t end)
    {
        for (uint32_t a = start; a <= end; ++a)
        {
            if (bits & 1) mlMonitor.clearReadWatch(static_cast<uint16_t>(a));
            if (bits & 2) mlMonitor.clearWriteWatch(static_cast<uint16_t>(a));
        }
    }
}

WatchCommand::WatchCommand() = default;

WatchCommand::~WatchCommand() = default;

int WatchCommand::order() const
{
    return 20;
}

std::string WatchCommand::name() const
{
    return "watch";
}

std::string WatchCommand::category() const
{
    return "Debugging";
}

std::string WatchCommand::shortHelp() const
{
    return "watch [read|write|both] [addr|start-end] - Manage watchpoints";
}

std::string WatchCommand::help() const
{
return R"(watch - manage memory watchpoints (reads, writes, or both)

```
Watchpoints stop emulation and return control to the monitor when the CPU
reads from or writes to a watched memory address.
```

Game Boy CPU memory map:
$0000-$3FFF   Cartridge ROM bank 0
$4000-$7FFF   Switchable cartridge ROM bank
$8000-$9FFF   Video RAM (VRAM)
$A000-$BFFF   External cartridge RAM
$C000-$CFFF   Work RAM bank 0
$D000-$DFFF   Work RAM bank 1
$E000-$FDFF   Echo RAM
$FE00-$FE9F   Object Attribute Memory (OAM)
$FEA0-$FEFF   Unusable memory area
$FF00-$FF7F   I/O registers
$FF80-$FFFE   High RAM (HRAM)
$FFFF         Interrupt Enable register

Listing:
watch                         List all watchpoints in a single table (R/W/RW)
watch list                    Same as above

Adding:
watch <addr>                  Add WRITE watch at <addr>
watch <start>-<end>           Add WRITE watches across a range
watch write <addr>            Add WRITE watch
watch write <start>-<end>     Add WRITE watches across a range
watch read  <addr>            Add READ watch
watch read  <start>-<end>     Add READ watches across a range
watch both  <addr>            Add READ and WRITE watches
watch both  <start>-<end>     Add READ and WRITE watches across a range
watch rw    <addr>            Alias for both
watch rw    <start>-<end>     Alias for both across a range

Clearing:
watch clear                         Clear ALL watchpoints
watch clear <addr>                  Clear READ and WRITE at <addr>
watch clear <start>-<end>           Clear READ and WRITE across a range
watch clear read  <addr>            Clear READ at <addr>
watch clear read  <start>-<end>     Clear READ across a range
watch clear write <addr>            Clear WRITE at <addr>
watch clear write <start>-<end>     Clear WRITE across a range

Examples:
watch write $C100            Watch a Work RAM variable for writes
watch read  $C100            Watch a Work RAM variable for reads
watch both  $C000-$C0FF      Watch a Work RAM range
watch read  $4000-$40FF      Watch reads from switchable cartridge ROM
watch write $8000-$9FFF      Watch CPU writes to VRAM
watch write $FE00-$FE9F      Watch CPU writes to sprite OAM
watch write $FF40            Watch writes to the LCDC register
watch both  $FF0F            Watch accesses to the interrupt flags
watch write $FFFF            Watch changes to interrupt enable

Notes:
Watchpoints observe CPU memory accesses made through the Game Boy memory
bus. Accesses may still be affected by cartridge banking, boot ROM mapping,
DMA transfers, and PPU memory-access restrictions.

```
A watchpoint on $4000-$7FFF observes the CPU address being accessed, not
the physical ROM bank currently mapped there.

Depending on the emulator's implementation, internal PPU VRAM/OAM accesses,
OAM DMA transfers, APU activity, timer updates, and other hardware-driven
operations may not trigger CPU memory watchpoints.
```

)";
}

void WatchCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    auto isMode = [](std::string m)
    {
        std::transform(m.begin(), m.end(), m.begin(), ::tolower);
        return m == "read" || m == "r" ||
               m == "write" || m == "w" ||
               m == "both" || m == "rw";
    };

    auto toBits = [](std::string m)->uint8_t
    {
        std::transform(m.begin(), m.end(), m.begin(), ::tolower);
        if (m == "read" || m == "r")   return 1; // R
        if (m == "write" || m == "w")  return 2; // W
        return 3; // both/rw
    };

    if (args.size() > 1 && isHelp(args[1]))
    {
        std::cout << help();
        return;
    }

    // LIST
    if (args.size() == 1 || args[1] == "list")
    {
        std::unordered_map<uint16_t, uint8_t> modes;

        for (auto a : mlMonitor.getReadWatchAddresses())  modes[a] |= 1;
        for (auto a : mlMonitor.getWriteWatchAddresses()) modes[a] |= 2;

        std::vector<uint16_t> addrs;
        addrs.reserve(modes.size());
        for (auto& kv : modes) addrs.push_back(kv.first);
        std::sort(addrs.begin(), addrs.end());

        if (addrs.empty())
        {
            std::cout << "(no watchpoints)\n";
            return;
        }

        for (auto a : addrs)
        {
            uint8_t m = modes[a];
            std::cout << "$" << std::hex << std::uppercase
                      << std::setw(4) << std::setfill('0') << a
                      << "   " << ((m & 1) ? "R" : "-")
                      << ((m & 2) ? "W" : "-") << "\n";
        }
        return;
    }

    // CLEAR
    if (args[1] == "clear")
    {
        if (args.size() == 2)
        {
            mlMonitor.clearAllReadWatches();
            mlMonitor.clearAllWriteWatches();
            return;
        }

        // watch clear <addr|range> => both
        if (args.size() == 3 && !isMode(args[2]))
        {
            try
            {
                WatchRange r = parseAddressOrRange(args[2]);
                applyClearRange(mlMonitor, 3, r.start, r.end);
            }
            catch (...)
            {
                std::cout << "Error: invalid address or range\n";
            }
            return;
        }

        // watch clear <mode> <addr|range>
        if (args.size() == 4 && isMode(args[2]))
        {
            try
            {
                uint8_t bits = toBits(args[2]);
                WatchRange r = parseAddressOrRange(args[3]);
                applyClearRange(mlMonitor, bits, r.start, r.end);
            }
            catch (...)
            {
                std::cout << "Error: invalid address or range\n";
            }
            return;
        }

        std::cout << help();
        return;
    }

    // ADD
    try
    {
        // watch <mode> <addr|range>
        if (isMode(args[1]))
        {
            if (args.size() < 3)
            {
                std::cout << "Error: missing address\n" << help();
                return;
            }

            uint8_t bits = toBits(args[1]);
            WatchRange r = parseAddressOrRange(args[2]);
            applyAddRange(mlMonitor, bits, r.start, r.end);
            return;
        }

        // watch <addr|range> (default WRITE for back-compat)
        WatchRange r = parseAddressOrRange(args[1]);
        applyAddRange(mlMonitor, 2, r.start, r.end);
    }
    catch (...)
    {
        std::cout << "Error: invalid address or range\n" << help();
    }
}
