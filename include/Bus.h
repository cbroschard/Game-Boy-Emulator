// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include "StateReader.h"
#include "StateWriter.h"

class APU;
class Cartridge;
class Joypad;
class Memory;
class MLMonitor;
class PPU;
class Timer;

class Bus
{
    public:
        Bus();
        virtual ~Bus();

        enum class Interrupt : uint8_t
        {
            VBlank,
            LCDStat,
            Timer,
            Serial,
            Joypad
        };

        inline void attachAPUInstance(APU* apu) { this->apu = apu; }
        inline void attachCartridgeInstance(Cartridge* cartridge) { this->cartridge = cartridge; }
        inline void attachJoypadInstance(Joypad* joypad) { this->joypad = joypad; }
        inline void attachMemoryInstance(Memory* memory) { this->memory = memory; }
        inline void attachMLMonitorInstance(MLMonitor* mlMonitor) { this->mlMonitor = mlMonitor; }
        inline void attachPPUInstance(PPU* ppu) { this->ppu = ppu; }
        inline void attachTimerInstance(Timer* timer) { this->timer = timer; }

        inline bool hasAPU() const { return apu ? 1 : 0; }
        inline bool hasCartridge() const { return cartridge ? 1 : 0; }
        inline bool hasJoypad() const { return joypad ? 1 : 0; }
        inline bool hasMemory() const { return memory ? 1 : 0; }
        inline bool hasPPU() const { return ppu ? 1 : 0; }
        inline bool hasTimer() const { return timer ? 1 : 0; }

        void reset();

        void saveState(StateWriter& wrtr) const;
        bool loadState(const StateReader::Chunk& chunk, StateReader& rdr);

        uint8_t read(uint16_t address);
        uint8_t readInternal(uint16_t address);
        void write(uint16_t address, uint8_t value);
        void writeInternal(uint16_t address, uint8_t value);

        void requestInterrupt(Interrupt interrupt);
        void clearInterrupt(Interrupt interrupt);

        inline uint8_t getInterruptFlags() const { return interruptStatus | 0xE0; }

        // Monitor access
        inline uint8_t peek(uint16_t address) { return readInternal(address); }
        inline void poke(uint16_t address, uint8_t value) { writeInternal(address, value); }

    protected:

    private:
        APU* apu;
        Cartridge* cartridge;
        Joypad* joypad;
        Memory* memory;
        MLMonitor* mlMonitor;
        PPU* ppu;
        Timer* timer;

        uint8_t readIO(uint16_t address);
        void writeIO(uint16_t address, uint8_t value);

        uint8_t interruptStatus;
};

#endif // BUS_H
