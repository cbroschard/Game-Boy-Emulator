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

        uint8_t readWRAM(uint16_t offset) const { return wram[offset]; }
        void writeWRAM(uint16_t offset, uint8_t value) { wram[offset] = value; }

        uint8_t readOAM(uint16_t offset) const { return oam[offset]; }
        void writeOAM(uint16_t offset, uint8_t value) { oam[offset] = value; }

        uint8_t readHRAM(uint16_t offset) const { return hram[offset]; }
        void writeHRAM(uint16_t offset, uint8_t value) { hram[offset] = value; }

    protected:

    private:
        std::array<uint8_t, 0x2000> vram{}; // 8 KB, $8000-$9FFF
        std::array<uint8_t, 0x2000> wram{}; // 8 KB, $C000-$DFFF
        std::array<uint8_t, 0xA0>   oam{};  // 160 bytes, $FE00-$FE9F
        std::array<uint8_t, 0x80>   io{};   // $FF00-$FF7F, temporary/simple start
        std::array<uint8_t, 0x7F>   hram{}; // 127 bytes, $FF80-$FFFE
};

#endif // MEMORY_H
