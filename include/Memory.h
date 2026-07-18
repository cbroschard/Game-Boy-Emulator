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
#include <string>
#include "common/HardwareMode.h"
#include "StateReader.h"
#include "StateWriter.h"

class Memory
{
    public:
        Memory();
        virtual ~Memory();

        void reset();

        void saveState(StateWriter& wrtr) const;
        bool loadState(const StateReader::Chunk& chunk, StateReader& rdr);

        uint8_t readWRAM(uint16_t offset) const;
        void writeWRAM(uint16_t offset, uint8_t value);

        uint8_t readSVBK() const;
        void writeSVBK(uint8_t value);

        inline uint8_t readHRAM(uint16_t offset) const { return hram[offset]; }
        inline void writeHRAM(uint16_t offset, uint8_t value) { hram[offset] = value; }

        inline uint8_t readIO(uint16_t offset) const { return io[offset]; }
        inline void writeIO(uint16_t offset, uint8_t value) { io[offset] = value; }

        inline uint8_t readBootROM(uint16_t offset) const { return (hardwareMode == HardwareMode::DMG) ? dmgBootRom[offset] : cgbBootRom[offset]; }
        inline void writeBootROMByte(uint16_t offset, uint8_t value) { (hardwareMode == HardwareMode::DMG) ? dmgBootRom[offset] = value
            : cgbBootRom[offset] = value; }

        inline uint8_t readIE() const { return interruptEnable; }
        inline void writeIE(uint8_t value) { interruptEnable = value; }

        inline bool isBootRomEnabled() const { return bootRomEnabled; }

        inline void disableBootRom() { bootRomEnabled = false; }

        inline void setHardwareMode(HardwareMode mode) { hardwareMode = mode; }

        inline HardwareMode getHardwareMode() const { return hardwareMode; }

        bool loadBIOS(const std::string& path);

        bool isBootRomMapped(uint16_t address) const;

    protected:

    private:
        std::array<uint8_t, 0x100> dmgBootRom{}; // DMG BIOS, $0000-$00FF
        std::array<uint8_t, 0x900> cgbBootRom{}; // CGH BIOS, E0000–00FF + $0200–08FF
        std::array<uint8_t, 0x1000> wramBank0{};
        std::array<std::array<uint8_t, 0x1000>, 7> wramBanks{};
        std::array<uint8_t, 0x80>   io{};   // $FF00-$FF7F,
        std::array<uint8_t, 0x7F>   hram{}; // 127 bytes, $FF80-$FFFE

        HardwareMode hardwareMode;

        bool bootRomEnabled;

        uint8_t interruptEnable;

        uint8_t selectedWRAMBank;

};

#endif // MEMORY_H
