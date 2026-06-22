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
    vram.fill(0x00);
    wram.fill(0x00);
    oam.fill(0x00);
    io.fill(0x00);
    hram.fill(0x00);

    bootRomEnabled  = true;
    interruptEnable = 0x00;
}

bool Memory::loadBIOS(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.read(reinterpret_cast<char*>(bootRom.data()), bootRom.size());

    return file.gcount() == static_cast<std::streamsize>(bootRom.size());
}
