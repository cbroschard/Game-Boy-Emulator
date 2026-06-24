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
    timer(nullptr),
    interruptStatus(0xE1)
{

}

Bus::~Bus()
{

}

void Bus::reset()
{
    interruptStatus = 0xE1; // $FF0F IF
}

uint8_t Bus::read(uint16_t address)
{
    if (!memory)
        return 0xFF;

    if (memory && memory->isBootRomEnabled() && address <= 0x00FF)
        return memory->readBootROM(address);

    if (address >= ROM_FIXED_START && address <= ROM_SWITCHABLE_END)
    {
        if (cartridge)
            return cartridge->readROM(address);

        return 0xFF;
    }

    if (address == 0xFF00)
    {
        if (joypad)
            return joypad->read();

        return 0xFF;
    }

    if (address >= EXTERNAL_RAM_START && address <= EXTERNAL_RAM_END)
    {
        if (cartridge)
            return cartridge->readRAM(address - EXTERNAL_RAM_START);

        return 0xFF;
    }

    if (address >= VRAM_START && address <= VRAM_END)
        return ppu ? ppu->readVRAM(address - VRAM_START) : 0xFF;

    if (address >= WRAM_START && address <= WRAM_END)
        return memory->readWRAM(address - WRAM_START);

    if (address >= ECHO_RAM_START && address <= ECHO_RAM_END)
        return memory->readWRAM(address - ECHO_RAM_START);

    if (address >= OAM_START && address <= OAM_END)
        return ppu ? ppu->readOAM(address - OAM_START) : 0xFF;

    if (address >= UNUSABLE_START && address <= UNUSABLE_END)
        return 0xFF;

    if (address >= IO_START && address <= IO_END)
        return readIO(address);

    if (address >= HRAM_START && address <= HRAM_END)
        return memory->readHRAM(address - HRAM_START);

    if (address == IE_REGISTER)
        return memory->readIE();

    return 0xFF;
}

void Bus::write(uint16_t address, uint8_t value)
{
    if (!memory)
        return;

    if (address >= ROM_FIXED_START && address <= ROM_SWITCHABLE_END)
    {
        if (cartridge)
            cartridge->writeROM(address, value); // MBC control write

        return;
    }

    if (address == 0xFF00)
    {
        if (joypad)
            joypad->write(value);

        return;
    }

    if (address >= EXTERNAL_RAM_START && address <= EXTERNAL_RAM_END)
    {
        if (cartridge)
            cartridge->writeRAM(address - EXTERNAL_RAM_START, value);

        return;
    }

    if (address >= VRAM_START && address <= VRAM_END)
    {
        if (ppu)
            ppu->writeVRAM(address - VRAM_START, value);

        return;
    }

    if (address >= WRAM_START && address <= WRAM_END)
    {
        memory->writeWRAM(address - WRAM_START, value);
        return;
    }

    if (address >= ECHO_RAM_START && address <= ECHO_RAM_END)
    {
        memory->writeWRAM(address - ECHO_RAM_START, value);
        return;
    }

    if (address >= OAM_START && address <= OAM_END)
    {
        if (ppu)
            ppu->writeOAM(address - OAM_START, value);

        return;
    }

    if (address >= UNUSABLE_START && address <= UNUSABLE_END)
        return;

    if (address >= IO_START && address <= IO_END)
    {
        writeIO(address, value);
        return;
    }

    if (address >= HRAM_START && address <= HRAM_END)
    {
        memory->writeHRAM(address - HRAM_START, value);
        return;
    }

    if (address == IE_REGISTER)
    {
        memory->writeIE(value);
        return;
    }
}

uint8_t Bus::readIO(uint16_t address)
{
    if (address >= 0xFF10 && address <= 0xFF3F)
        return apu ? apu->readRegister(address) : 0xFF;

    switch (address)
    {
        case LCDC_REGISTER:
        case STAT_REGISTER:
        case SCY_REGISTER:
        case SCX_REGISTER:
        case LY_REGISTER:
        case LYC_REGISTER:
        case BGP_REGISTER:
        case OBP0_REGISTER:
        case OBP1_REGISTER:
        case WY_REGISTER:
        case WX_REGISTER:
            return ppu ? ppu->readRegister(address) : 0xFF;

        case IF_REGISTER:
            return interruptStatus;

        default:
            return memory->readIO(address - IO_START);
    }
}

void Bus::writeIO(uint16_t address, uint8_t value)
{
    if (address >= 0xFF10 && address <= 0xFF3F)
    {
        if (apu)
            apu->writeRegister(address, value);

        memory->writeIO(address - IO_START, value);
        return;
    }

    switch (address)
    {
        case LCDC_REGISTER:
        case STAT_REGISTER:
        case SCY_REGISTER:
        case SCX_REGISTER:
        case LY_REGISTER:
        case LYC_REGISTER:
        case BGP_REGISTER:
        case OBP0_REGISTER:
        case OBP1_REGISTER:
        case WY_REGISTER:
        case WX_REGISTER:
            if (ppu)
                ppu->writeRegister(address, value);
                return;

        case DMA_REGISTER:
        {
            memory->writeIO(address - IO_START, value);

            uint16_t source = static_cast<uint16_t>(value) << 8;

            for (uint16_t i = 0; i < 0xA0; i++)
            {
                uint8_t byte = read(source + i);

                if (ppu)
                    ppu->writeOAM(i, byte);
            }

            return;
        }

        case IF_REGISTER:
            interruptStatus = value | 0xE0;
            memory->writeIO(address - IO_START, interruptStatus);
            return;

        case 0xFF50:
            memory->writeIO(address - IO_START, value);

            if (value != 0)
                memory->disableBootRom();

            return;

        default:
            memory->writeIO(address - IO_START, value);
            return;
    }
}
