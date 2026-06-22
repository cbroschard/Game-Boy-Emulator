// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <cstdint>

class Memory
{
    public:
        Memory();
        virtual ~Memory();

        void reset();

        uint8_t readVRAM(uint16_t address);
        void writeVRAM(uint16_t address, uint8_t value);

        inline uint8_t readWRAM(uint16_t offset) const { return wram[offset]; }
        inline void writeWRAM(uint16_t offset, uint8_t value) { wram[offset] = value; }

        inline uint8_t readOAM(uint16_t offset) const { return oam[offset]; }
        inline void writeOAM(uint16_t offset, uint8_t value) { oam[offset] = value; }

        inline uint8_t readHRAM(uint16_t offset) const { return hram[offset]; }
        inline void writeHRAM(uint16_t offset, uint8_t value) { hram[offset] = value; }

        inline uint8_t readIO(uint16_t offset) const { return io[offset]; }
        inline void writeIO(uint16_t offset, uint8_t value) { io[offset] = value; }

        inline uint8_t readBootROM(uint16_t offset) const { return bootRom[offset]; }
        inline void writeBootROMByte(uint16_t offset, uint8_t value) { bootRom[offset] = value; }

        inline uint8_t readIE() const { return interruptEnable; }
        inline void writeIE(uint8_t value) { interruptEnable = value; }

        inline bool isBootRomEnabled() const { return bootRomEnabled; }

        inline void disableBootRom() { bootRomEnabled = false; }

    protected:

    private:
        std::array<uint8_t, 0x100> bootRom{}; // DMG BIOS, $0000-$00FF
        std::array<uint8_t, 0x2000> vram{}; // 8 KB, $8000-$9FFF
        std::array<uint8_t, 0x2000> wram{}; // 8 KB, $C000-$DFFF
        std::array<uint8_t, 0xA0>   oam{};  // 160 bytes, $FE00-$FE9F
        std::array<uint8_t, 0x80>   io{};   // $FF00-$FF7F,
        std::array<uint8_t, 0x7F>   hram{}; // 127 bytes, $FF80-$FFFE

        bool bootRomEnabled;
        uint8_t interruptEnable;
};

#endif // MEMORY_H
