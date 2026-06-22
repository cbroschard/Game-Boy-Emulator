// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "APU.h"
#include "Bus.h"
#include "Cartridge.h"
#include "Joypad.h"
#include "Memory.h"
#include "PPU.h"

Bus::Bus() :
    apu(nullptr),
    cartridge(nullptr),
    joypad(nullptr),
    memory(nullptr),
    ppu(nullptr),
    interruptEnable(0x00),
    interruptStatus(0xE1)
{

}

Bus::~Bus()
{

}

void Bus::reset()
{
    interruptEnable = 0x00; // $FFFF IE
    interruptStatus = 0xE1; // $FF0F IF
}

uint8_t Bus::read(uint16_t address)
{
    return 0xFF;
}

void Bus::write(uint16_t address, uint8_t value)
{

}
