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
#include "common/IORegisters.h"
#include "Joypad.h"
#include "Memory.h"
#include "common/MemoryMap.h"
#include "MLMonitor.h"
#include "PPU.h"
#include "Timer.h"

Bus::Bus() :
    apu(nullptr),
    cartridge(nullptr),
    joypad(nullptr),
    memory(nullptr),
    mlMonitor(nullptr),
    ppu(nullptr),
    timer(nullptr),
    interruptStatus(0xE1),
    hdma1(0xFF),
    hdma2(0xFF),
    hdma3(0xFF),
    hdma4(0xFF),
    hdma5(0xFF),
    hblankDMAActive(false),
    hdmaSource(0),
    hdmaDestination(0),
    hdmaBlocksRemaining(0)
{

}

Bus::~Bus()
{

}

void Bus::reset()
{
    interruptStatus = 0xE1;

    hdma1 = 0xFF;
    hdma2 = 0xFF;
    hdma3 = 0xFF;
    hdma4 = 0xFF;
    hdma5 = 0xFF;

    hblankDMAActive = false;
    hdmaSource = 0;
    hdmaDestination = 0;
    hdmaBlocksRemaining = 0;
}

void Bus::saveState(StateWriter& wrtr) const
{
    wrtr.beginChunk("BUS0");

    // Version
    wrtr.writeU32(1);

    wrtr.writeU8(interruptStatus);

    wrtr.writeU8(hdma1);
    wrtr.writeU8(hdma2);
    wrtr.writeU8(hdma3);
    wrtr.writeU8(hdma4);
    wrtr.writeU8(hdma5);

    wrtr.writeBool(hblankDMAActive);
    wrtr.writeU16(hdmaSource);
    wrtr.writeU16(hdmaDestination);
    wrtr.writeU8(hdmaBlocksRemaining);

    wrtr.endChunk();
}

bool Bus::loadState(const StateReader::Chunk& chunk, StateReader& rdr)
{
    rdr.enterChunkPayload(chunk);

    if (std::memcmp(chunk.tag, "BUS0", 4) != 0)     { rdr.exitChunkPayload(chunk); return false; }

    uint32_t version = 0;
    if (!rdr.readU32(version))                      { rdr.exitChunkPayload(chunk); return false; }
    if (version != 1)                               { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(interruptStatus))               { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(hdma1))                         { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(hdma2))                         { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(hdma3))                         { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(hdma4))                         { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(hdma5))                         { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readBool(hblankDMAActive))             { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU16(hdmaSource))                   { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU16(hdmaDestination))              { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(hdmaBlocksRemaining))           { rdr.exitChunkPayload(chunk); return false; }

    interruptStatus |= 0xE0;

    rdr.exitChunkPayload(chunk);

    return true;
}

uint8_t Bus::read(uint16_t address)
{
    const uint8_t value = readInternal(address);

    if (mlMonitor && mlMonitor->checkWatchRead(address, value))
        mlMonitor->requestBreak();

    return value;
}

uint8_t Bus::readInternal(uint16_t address)
{
    if (!memory)
        return 0xFF;

    if (memory->isBootRomMapped(address))
        return memory->readBootROM(address);

    if (address >= MemoryMap::ROM_FIXED_START && address <= MemoryMap::ROM_SWITCHABLE_END)
    {
        if (cartridge)
            return cartridge->readROM(address);

        return 0xFF;
    }

    if (address >= MemoryMap::VRAM_START && address <= MemoryMap::VRAM_END)
        return ppu ? ppu->readVRAM(address - MemoryMap::VRAM_START) : 0xFF;

    if (address >= MemoryMap::EXTERNAL_RAM_START && address <= MemoryMap::EXTERNAL_RAM_END)
    {
        if (cartridge)
            return cartridge->readRAM(address - MemoryMap::EXTERNAL_RAM_START);

        return 0xFF;
    }

    if (address >= MemoryMap::WRAM_START && address <= MemoryMap::WRAM_END)
        return memory->readWRAM(address - MemoryMap::WRAM_START);

    if (address >= MemoryMap::ECHO_RAM_START && address <= MemoryMap::ECHO_RAM_END)
    {
        const uint16_t mirroredAddress = static_cast<uint16_t>(address - 0x2000);

        return memory->readWRAM(static_cast<uint16_t>(mirroredAddress - MemoryMap::WRAM_START));
    }

    if (address >= MemoryMap::OAM_START && address <= MemoryMap::OAM_END)
        return ppu ? ppu->readOAM(address - MemoryMap::OAM_START) : 0xFF;

    if (address >= MemoryMap::UNUSABLE_START && address <= MemoryMap::UNUSABLE_END)
        return 0xFF;

    if (address >= MemoryMap::IO_START && address <= MemoryMap::IO_END)
        return readIO(address);

    if (address >= MemoryMap::HRAM_START && address <= MemoryMap::HRAM_END)
        return memory->readHRAM(address - MemoryMap::HRAM_START);

    if (address == IORegisters::Interrupt::IE)
        return memory->readIE();

    return 0xFF;
}

void Bus::write(uint16_t address, uint8_t value)
{
    if (!memory)
        return;

    writeInternal(address, value);

    if (mlMonitor && mlMonitor->checkWatchWrite(address, value))
        mlMonitor->requestBreak();
}

void Bus::writeInternal(uint16_t address, uint8_t value)
{
    if (!memory)
        return;

    if (address >= MemoryMap::ROM_FIXED_START && address <= MemoryMap::ROM_SWITCHABLE_END)
    {
        if (cartridge)
            cartridge->writeROM(address, value); // MBC control write

        return;
    }

    if (address >= MemoryMap::EXTERNAL_RAM_START && address <= MemoryMap::EXTERNAL_RAM_END)
    {
        if (cartridge)
            cartridge->writeRAM(address - MemoryMap::EXTERNAL_RAM_START, value);

        return;
    }

    if (address >= MemoryMap::VRAM_START && address <= MemoryMap::VRAM_END)
    {
        if (ppu)
            ppu->writeVRAM(address - MemoryMap::VRAM_START, value);

        return;
    }

    if (address >= MemoryMap::WRAM_START && address <= MemoryMap::WRAM_END)
    {
        memory->writeWRAM(address - MemoryMap::WRAM_START, value);
        return;
    }

    if (address >= MemoryMap::ECHO_RAM_START && address <= MemoryMap::ECHO_RAM_END)
    {
        const uint16_t mirroredAddress =  static_cast<uint16_t>(address - 0x2000);

        memory->writeWRAM(static_cast<uint16_t>(mirroredAddress - MemoryMap::WRAM_START), value);

        return;
    }

    if (address >= MemoryMap::OAM_START && address <= MemoryMap::OAM_END)
    {
        if (ppu)
            ppu->writeOAM(address - MemoryMap::OAM_START, value);

        return;
    }

    if (address >= MemoryMap::UNUSABLE_START && address <= MemoryMap::UNUSABLE_END)
        return;

    if (address >= MemoryMap::IO_START && address <= MemoryMap::IO_END)
    {
        writeIO(address, value);
        return;
    }

    if (address >= MemoryMap::HRAM_START && address <= MemoryMap::HRAM_END)
    {
        memory->writeHRAM(address - MemoryMap::HRAM_START, value);
        return;
    }

    if (address == IORegisters::Interrupt::IE)
    {
        memory->writeIE(value);
        return;
    }
}

uint8_t Bus::readIO(uint16_t address)
{
    if (address >= IORegisters::APU::Start && address <= IORegisters::APU::End)
        return apu ? apu->readRegister(address) : 0xFF;

    switch (address)
    {
        case IORegisters::CGB::KEY0:
        {
            if (!memory)
                return 0xFF;

            return memory->getHardwareMode() == HardwareMode::CGB ? 0x80 : 0x04;
        }

        case IORegisters::CGB::KEY1:
        {
            if (!memory || memory->getHardwareMode() != HardwareMode::CGB)
                return 0xFF;

            const uint8_t stored = memory->readIO(address - MemoryMap::IO_START);

            return static_cast<uint8_t>(0x7E | (stored & 0x01));
        }

        case IORegisters::CGB::SVBK:
            return memory ? memory->readSVBK() : 0xFF;

        case IORegisters::Joypad::JOYP:
            return joypad ? joypad->read() : 0xFF;

        case IORegisters::Timer::DIV:
        case IORegisters::Timer::TIMA:
        case IORegisters::Timer::TMA:
        case IORegisters::Timer::TAC:
            return timer ? timer->readRegister(address) : 0xFF;

        case IORegisters::PPU::LCDC:
        case IORegisters::PPU::STAT:
        case IORegisters::PPU::SCY:
        case IORegisters::PPU::SCX:
        case IORegisters::PPU::LY:
        case IORegisters::PPU::LYC:
        case IORegisters::PPU::BGP:
        case IORegisters::PPU::OBP0:
        case IORegisters::PPU::OBP1:
        case IORegisters::PPU::WY:
        case IORegisters::PPU::WX:
        case IORegisters::PPU::VBK:
        case IORegisters::PPU::BGPI:
        case IORegisters::PPU::BGPD:
        case IORegisters::PPU::OBPI:
        case IORegisters::PPU::OBPD:
        case IORegisters::PPU::OPRI:
            return ppu ? ppu->readRegister(address) : 0xFF;

        case IORegisters::PPU::HDMA1:
            return hdma1;

        case IORegisters::PPU::HDMA2:
            return static_cast<uint8_t>(hdma2 | 0x0F);

        case IORegisters::PPU::HDMA3:
            return static_cast<uint8_t>(hdma3 | 0xE0);

        case IORegisters::PPU::HDMA4:
            return static_cast<uint8_t>(hdma4 | 0x0F);

        case IORegisters::PPU::HDMA5:
        {
            if (hblankDMAActive)
            {
                return static_cast<uint8_t>(
                    (hdmaBlocksRemaining - 1) & 0x7F
                );
            }

            if (hdmaBlocksRemaining == 0)
                return 0xFF;

            return static_cast<uint8_t>(
                0x80 |
                ((hdmaBlocksRemaining - 1) & 0x7F)
            );
        }

        case IORegisters::PPU::DMA:
            return memory->readIO(address - MemoryMap::IO_START);

        case IORegisters::Interrupt::IF:
            return interruptStatus | 0xE0;

        default:
            return memory->readIO(address - MemoryMap::IO_START);
    }
}

void Bus::writeIO(uint16_t address, uint8_t value)
{
    if (address >= IORegisters::APU::Start && address <= IORegisters::APU::End)
    {
        if (apu)
            apu->writeRegister(address, value);

        memory->writeIO(address - MemoryMap::IO_START, value);
        return;
    }

    switch (address)
    {
        case IORegisters::CGB::KEY0:
        {
            // KEY0 is controlled during CGB boot.
            // The emulator already selected the hardware mode.
            return;
        }

        case IORegisters::CGB::KEY1:
        {
            if (memory->getHardwareMode() == HardwareMode::CGB)
                memory->writeIO(address - MemoryMap::IO_START, value & 0x01);

            return;
        }

        case IORegisters::CGB::SVBK:
        {
            if (memory)
                memory->writeSVBK(value);

            return;
        }

        case IORegisters::Timer::DIV:
        case IORegisters::Timer::TIMA:
        case IORegisters::Timer::TMA:
        case IORegisters::Timer::TAC:
        {
            if (timer)
                timer->writeRegister(address, value);

            return;
        }

        case IORegisters::PPU::LCDC:
        case IORegisters::PPU::STAT:
        case IORegisters::PPU::SCY:
        case IORegisters::PPU::SCX:
        case IORegisters::PPU::LY:
        case IORegisters::PPU::LYC:
        case IORegisters::PPU::BGP:
        case IORegisters::PPU::OBP0:
        case IORegisters::PPU::OBP1:
        case IORegisters::PPU::WY:
        case IORegisters::PPU::WX:
        case IORegisters::PPU::VBK:
        case IORegisters::PPU::BGPI:
        case IORegisters::PPU::BGPD:
        case IORegisters::PPU::OBPI:
        case IORegisters::PPU::OBPD:
        case IORegisters::PPU::OPRI:
        {
            if (ppu)
                ppu->writeRegister(address, value);

            return;
        }

        case IORegisters::PPU::HDMA1:
        {
            hdma1 = value;
            updateHDMAAddresses();
            return;
        }

        case IORegisters::PPU::HDMA2:
        {
            hdma2 = value & 0xF0;
            updateHDMAAddresses();
            return;
        }

        case IORegisters::PPU::HDMA3:
        {
            hdma3 = value & 0x1F;
            updateHDMAAddresses();
            return;
        }

        case IORegisters::PPU::HDMA4:
        {
            hdma4 = value & 0xF0;
            updateHDMAAddresses();
            return;
        }

        case IORegisters::PPU::HDMA5:
        {
            /*
             * Writing bit 7 clear while HBlank DMA is active
             * stops the active transfer.
             */
            if (hblankDMAActive && (value & 0x80) == 0)
            {
                hblankDMAActive = false;

                hdma5 =
                    static_cast<uint8_t>(
                        0x80 |
                        ((hdmaBlocksRemaining - 1) & 0x7F)
                    );

                return;
            }

            hdmaBlocksRemaining =
                static_cast<uint8_t>(
                    (value & 0x7F) + 1
                );

            updateHDMAAddresses();

            if ((value & 0x80) == 0)
            {
                // General-purpose DMA.
                while (hdmaBlocksRemaining > 0)
                {
                    transferVRAMDMABlock();
                    hdmaBlocksRemaining--;
                }

                hblankDMAActive = false;
                hdma5 = 0xFF;
            }
            else
            {
                // HBlank DMA.
                hblankDMAActive = true;

                hdma5 =
                    static_cast<uint8_t>(
                        (hdmaBlocksRemaining - 1) & 0x7F
                    );
            }

            return;
        }

        case IORegisters::PPU::DMA:
        {
            memory->writeIO(address - MemoryMap::IO_START, value);

            const uint16_t source =
                static_cast<uint16_t>(value) << 8;

            for (uint16_t i = 0; i < 0xA0; i++)
            {
                const uint8_t byte =
                    readInternal(static_cast<uint16_t>(source + i));

                if (ppu)
                    ppu->writeOAM(i, byte);
            }

            return;
        }

        case IORegisters::Interrupt::IF:
        {
            interruptStatus = value | 0xE0;
            memory->writeIO(
                address - MemoryMap::IO_START,
                interruptStatus
            );

            return;
        }

        case IORegisters::Joypad::JOYP:
        {
            if (joypad)
                joypad->write(value);

            return;
        }

        case IORegisters::Boot::Disable:
        {
            memory->writeIO(address - MemoryMap::IO_START, value);

            if (value != 0)
                memory->disableBootRom();

            return;
        }

        default:
            memory->writeIO(address - MemoryMap::IO_START, value);
            return;
    }
}

void Bus::requestInterrupt(Interrupt interrupt)
{
    const uint8_t bit = static_cast<uint8_t>(interrupt);

    if (bit > 4)
        return;

    interruptStatus |= static_cast<uint8_t>(1 << bit);

    // Upper bits of IF read back as 1 on DMG.
    interruptStatus |= 0xE0;
}

void Bus::clearInterrupt(Interrupt interrupt)
{
    const uint8_t bit = static_cast<uint8_t>(interrupt);

    if (bit > 4)
        return;

    interruptStatus &= static_cast<uint8_t>(~(1 << bit));

    // Keep unused bits high.
    interruptStatus |= 0xE0;
}

void Bus::updateHDMAAddresses()
{
    hdmaSource =
        static_cast<uint16_t>(
            (static_cast<uint16_t>(hdma1) << 8) |
            (hdma2 & 0xF0)
        );

    hdmaDestination =
        static_cast<uint16_t>(
            0x8000 |
            (static_cast<uint16_t>(hdma3 & 0x1F) << 8) |
            (hdma4 & 0xF0)
        );
}

void Bus::transferVRAMDMABlock()
{
    if (!ppu)
        return;

    for (uint16_t i = 0; i < 0x10; i++)
    {
        const uint8_t value =
            readInternal(
                static_cast<uint16_t>(
                    hdmaSource + i
                )
            );

        const uint16_t destinationAddress =
            static_cast<uint16_t>(
                hdmaDestination + i
            );

        const uint16_t vramOffset =
            static_cast<uint16_t>(
                destinationAddress -
                MemoryMap::VRAM_START
            );

        ppu->writeVRAMFromDMA(
            vramOffset,
            value
        );
    }

    hdmaSource =
        static_cast<uint16_t>(
            hdmaSource + 0x10
        );

    hdmaDestination =
        static_cast<uint16_t>(
            0x8000 |
            ((hdmaDestination + 0x10) & 0x1FF0)
        );

    hdma1 =
        static_cast<uint8_t>(
            hdmaSource >> 8
        );

    hdma2 =
        static_cast<uint8_t>(
            hdmaSource & 0xF0
        );

    hdma3 =
        static_cast<uint8_t>(
            (hdmaDestination >> 8) & 0x1F
        );

    hdma4 =
        static_cast<uint8_t>(
            hdmaDestination & 0xF0
        );
}

void Bus::onPPUHBlank()
{
    if (!hblankDMAActive)
        return;

    if (hdmaBlocksRemaining == 0)
    {
        hblankDMAActive = false;
        hdma5 = 0xFF;
        return;
    }

    transferVRAMDMABlock();

    hdmaBlocksRemaining--;

    if (hdmaBlocksRemaining == 0)
    {
        hblankDMAActive = false;
        hdma5 = 0xFF;
    }
    else
    {
        hdma5 =
            static_cast<uint8_t>(
                (hdmaBlocksRemaining - 1) &
                0x7F
            );
    }
}
