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
    bootRomEnabled(true),
    interruptEnable(0x00)
{
    reset();
}

Memory::~Memory()
{

}

void Memory::reset()
{
    wram.fill(0x00);
    io.fill(0x00);
    hram.fill(0x00);

    bootRomEnabled  = true;
    interruptEnable = 0x00;
}

void Memory::saveState(StateWriter& wrtr) const
{
    wrtr.beginChunk("MEM0");

    // Version
    wrtr.writeU32(1);

    wrtr.writeBool(bootRomEnabled);

    wrtr.writeU8(interruptEnable);

    wrtr.writeArrayU8(wram.data(), wram.size());
    wrtr.writeArrayU8(io.data(), io.size());
    wrtr.writeArrayU8(hram.data(), hram.size());

    wrtr.endChunk();
}

bool Memory::loadState(const StateReader::Chunk& chunk, StateReader& rdr)
{
    rdr.enterChunkPayload(chunk);

    if (std::memcmp(chunk.tag, "MEM0", 4) != 0)     { rdr.exitChunkPayload(chunk); return false; }

    uint32_t version = 0;
    if (!rdr.readU32(version))                      { rdr.exitChunkPayload(chunk); return false; }
    if (version != 1)                               { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readBool(bootRomEnabled))              { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(interruptEnable))               { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readArrayU8(wram.data(), wram.size())) { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readArrayU8(io.data(), io.size()))     { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readArrayU8(hram.data(), hram.size())) { rdr.exitChunkPayload(chunk); return false; }

    rdr.exitChunkPayload(chunk);

    return true;
}

bool Memory::loadBIOS(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.read(reinterpret_cast<char*>(bootRom.data()), bootRom.size());

    return file.gcount() == static_cast<std::streamsize>(bootRom.size());
}
