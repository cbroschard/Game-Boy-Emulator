// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <fstream>
#include "Memory.h"

Memory::Memory() :
    hardwareMode(HardwareMode::DMG),
    bootRomEnabled(true),
    interruptEnable(0x00),
    selectedWRAMBank(1)
{
    reset();
}

Memory::~Memory()
{

}

void Memory::reset()
{
    wramBank0.fill(0x00);

    for (auto& bank : wramBanks)
        bank.fill(0x00);

    io.fill(0x00);
    hram.fill(0x00);

    bootRomEnabled      = true;

    interruptEnable     = 0x00;

    selectedWRAMBank    = 1;
}

void Memory::saveState(StateWriter& wrtr) const
{
    wrtr.beginChunk("MEM0");

    // Version
    wrtr.writeU32(1);

    wrtr.writeBool(bootRomEnabled);

    wrtr.writeU8(interruptEnable);

    wrtr.writeArrayU8(wramBank0.data(), wramBank0.size());

    for (const auto& bank : wramBanks)
    {
        wrtr.writeArrayU8(
            bank.data(),
            bank.size()
        );
    }

    wrtr.writeU8(selectedWRAMBank);

    wrtr.writeArrayU8(io.data(), io.size());
    wrtr.writeArrayU8(hram.data(), hram.size());

    wrtr.endChunk();
}

bool Memory::loadState(const StateReader::Chunk& chunk, StateReader& rdr)
{
    rdr.enterChunkPayload(chunk);

    if (std::memcmp(chunk.tag, "MEM0", 4) != 0)                 { rdr.exitChunkPayload(chunk); return false; }

    uint32_t version = 0;
    if (!rdr.readU32(version))                                  { rdr.exitChunkPayload(chunk); return false; }
    if (version != 1)                                           { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readBool(bootRomEnabled))                          { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(interruptEnable))                           { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readArrayU8(wramBank0.data(), wramBank0.size()))   { rdr.exitChunkPayload(chunk); return false; }

    for (auto& bank : wramBanks)
    {
        if (!rdr.readArrayU8(bank.data(), bank.size()))         { rdr.exitChunkPayload(chunk); return false; }
    }

    if (!rdr.readU8(selectedWRAMBank))                          { rdr.exitChunkPayload(chunk); return false; }

    selectedWRAMBank &= 0x07;

    if (selectedWRAMBank == 0)
        selectedWRAMBank = 1;
    if (!rdr.readArrayU8(io.data(), io.size()))                 { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readArrayU8(hram.data(), hram.size()))             { rdr.exitChunkPayload(chunk); return false; }

    rdr.exitChunkPayload(chunk);

    return true;
}

uint8_t Memory::readWRAM(uint16_t offset) const
{
    if (offset >= 0x2000)
        return 0xFF;

    // $C000-$CFFF: fixed bank 0
    if (offset < 0x1000)
        return wramBank0[offset];

    // $D000-$DFFF: switchable bank
    const uint16_t bankOffset =
        static_cast<uint16_t>(offset - 0x1000);

    if (hardwareMode == HardwareMode::DMG)
        return wramBanks[0][bankOffset];

    uint8_t bank =
        static_cast<uint8_t>(
            selectedWRAMBank & 0x07
        );

    if (bank == 0)
        bank = 1;

    return wramBanks[bank - 1][bankOffset];
}

void Memory::writeWRAM(uint16_t offset, uint8_t value)
{
    if (offset >= 0x2000)
        return;

    // $C000-$CFFF: fixed bank 0
    if (offset < 0x1000)
    {
        wramBank0[offset] = value;
        return;
    }

    // $D000-$DFFF: switchable bank
    const uint16_t bankOffset =
        static_cast<uint16_t>(offset - 0x1000);

    if (hardwareMode == HardwareMode::DMG)
    {
        wramBanks[0][bankOffset] = value;
        return;
    }

    uint8_t bank =
        static_cast<uint8_t>(
            selectedWRAMBank & 0x07
        );

    if (bank == 0)
        bank = 1;

    wramBanks[bank - 1][bankOffset] = value;
}

uint8_t Memory::readSVBK() const
{
    if (hardwareMode != HardwareMode::CGB)
        return 0xFF;

    return static_cast<uint8_t>(
        0xF8 |
        (selectedWRAMBank & 0x07)
    );
}

void Memory::writeSVBK(uint8_t value)
{
    if (hardwareMode != HardwareMode::CGB)
        return;

    selectedWRAMBank =
        static_cast<uint8_t>(
            value & 0x07
        );

    // SVBK value 0 selects bank 1.
    if (selectedWRAMBank == 0)
        selectedWRAMBank = 1;
}

bool Memory::loadBIOS(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    if (hardwareMode == HardwareMode::DMG)
    {
        file.read(reinterpret_cast<char*>(dmgBootRom.data()), dmgBootRom.size());
        return file.gcount() == static_cast<std::streamsize>(dmgBootRom.size());
    }

    file.read(reinterpret_cast<char*>(cgbBootRom.data()), cgbBootRom.size());
    return file.gcount() == static_cast<std::streamsize>(cgbBootRom.size());
}

bool Memory::isBootRomMapped(uint16_t address) const
{
    if (!bootRomEnabled)
        return false;

    if (hardwareMode == HardwareMode::DMG)
        return address <= 0x00FF;

    // CGB BIOS mapping:
    // $0000-$00FF = BIOS
    // $0100-$01FF = cartridge
    // $0200-$08FF = BIOS
    return address <= 0x00FF ||
           (address >= 0x0200 && address <= 0x08FF);
}
