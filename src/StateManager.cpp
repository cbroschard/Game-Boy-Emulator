// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.

#include "StateManager.h"
#include "APU.h"
#include "Bus.h"
#include "Cartridge.h"
#include "CPU.h"
#include "Joypad.h"
#include "Memory.h"
#include "PPU.h"
#include "Timer.h"

StateManager::StateManager()

{

}

StateManager::~StateManager() = default;

bool StateManager::save(const std::string& path)
{
    StateWriter wrtr(kStateVersion);
    wrtr.beginFile();

    wrtr.beginChunk("SYS0");

    // Version
    wrtr.writeU32(1);

    wrtr.endChunk();

    // Save device chunks
    apu->saveState(wrtr);

    bus->saveState(wrtr);

    cartridge->saveState(wrtr);

    cpu->saveState(wrtr);

    joypad->saveState(wrtr);

    memory->saveState(wrtr);

    ppu->saveState(wrtr);

    timer->saveState(wrtr);

    // Write file
    return wrtr.writeToFile(path);
}

bool StateManager::load(const std::string& path)
{
    StateReader rdr;

    // Try to read given file
    const bool loaded   = rdr.loadFromFile(path);
    const bool validate = rdr.readFileHeader();

    // Fail if we can't load or validate the file
    if (!loaded || !validate)
    {
        #ifdef Debug
        std::cout << "Unable to load .sav file!\n";
        #endif
        return false;
    }

    // Process the first chunk
    StateReader::Chunk chunk;
    if (!rdr.nextChunk(chunk))
        return false;

    if (std::memcmp(chunk.tag, "SYS0", 4) != 0)
        return false;

    rdr.enterChunkPayload(chunk);

    uint32_t version = 0;
    if (!rdr.readU32(version))          { rdr.exitChunkPayload(chunk); return false; }
    if (version != 1)                   { rdr.exitChunkPayload(chunk); return false; }

    rdr.exitChunkPayload(chunk);

    while (rdr.nextChunk(chunk))
    {
        if (std::memcmp(chunk.tag, "APU0", 4) == 0)
        {
            if (!apu->loadState(chunk, rdr))
                return false;
        }
        else if (std::memcmp(chunk.tag, "BUS0", 4) == 0)
        {
            if (!bus->loadState(chunk, rdr))
                return false;
        }
        else if (std::memcmp(chunk.tag, "CART", 4) == 0)
        {
            if (!cartridge->loadState(chunk, rdr))
                return false;
        }
        else if (std::memcmp(chunk.tag, "CPU0", 4) == 0)
        {
            if (!cpu->loadState(chunk, rdr))
                return false;
        }
        else if (std::memcmp(chunk.tag, "JOY0", 4) == 0)
        {
            if (!joypad->loadState(chunk, rdr))
                return false;
        }
        else if (std::memcmp(chunk.tag, "MEM0", 4) == 0)
        {
            if (!memory->loadState(chunk, rdr))
                return false;
        }
        else if (std::memcmp(chunk.tag, "PPU0", 4) == 0)
        {
            if (!ppu->loadState(chunk, rdr))
                return false;
        }
        else if (std::memcmp(chunk.tag, "TMR0", 4) == 0)
        {
            if (!timer->loadState(chunk, rdr))
                return false;
        }
        else
        {
            rdr.skipChunk(chunk);
        }
    }

    return true;
}
