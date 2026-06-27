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

class APU;
class Cartridge;
class Joypad;
class Memory;
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
        inline void attachPPUInstance(PPU* ppu) { this->ppu = ppu; }
        inline void attachTimerInstance(Timer* timer) { this->timer = timer; }

        inline bool hasAPU() const { return apu ? 1 : 0; }
        inline bool hasCartridge() const { return cartridge ? 1 : 0; }
        inline bool hasJoypad() const { return joypad ? 1 : 0; }
        inline bool hasMemory() const { return memory ? 1 : 0; }
        inline bool hasPPU() const { return ppu ? 1 : 0; }
        inline bool hasTimer() const { return timer ? 1 : 0; }

        void reset();

        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t value);

        void requestInterrupt(Interrupt interrupt);
        void clearInterrupt(Interrupt interrupt);

        inline uint8_t getInterruptFlags() const { return interruptStatus | 0xE0; }

    protected:

    private:
        APU* apu;
        Cartridge* cartridge;
        Joypad* joypad;
        Memory* memory;
        PPU* ppu;
        Timer* timer;

        // Memory locations
        static constexpr uint16_t ROM_FIXED_START       = 0x0000;
        static constexpr uint16_t ROM_FIXED_END         = 0x3FFF;

        static constexpr uint16_t ROM_SWITCHABLE_START  = 0x4000;
        static constexpr uint16_t ROM_SWITCHABLE_END    = 0x7FFF;

        static constexpr uint16_t VRAM_START            = 0x8000;
        static constexpr uint16_t VRAM_END              = 0x9FFF;

        static constexpr uint16_t EXTERNAL_RAM_START    = 0xA000;
        static constexpr uint16_t EXTERNAL_RAM_END      = 0xBFFF;

        static constexpr uint16_t WRAM_START            = 0xC000;
        static constexpr uint16_t WRAM_END              = 0xDFFF;

        static constexpr uint16_t ECHO_RAM_START        = 0xE000;
        static constexpr uint16_t ECHO_RAM_END          = 0xFDFF;

        static constexpr uint16_t OAM_START             = 0xFE00;
        static constexpr uint16_t OAM_END               = 0xFE9F;

        static constexpr uint16_t UNUSABLE_START        = 0xFEA0;
        static constexpr uint16_t UNUSABLE_END          = 0xFEFF;

        static constexpr uint16_t HRAM_START            = 0xFF80;
        static constexpr uint16_t HRAM_END              = 0xFFFE;

        // IO constants
        static constexpr uint16_t IO_START = 0xFF00;
        static constexpr uint16_t IO_END   = 0xFF7F;

        static constexpr uint16_t JOYPAD_REGISTER = 0xFF00;

        static constexpr uint16_t DIV_REGISTER  = 0xFF04;
        static constexpr uint16_t TIMA_REGISTER = 0xFF05;
        static constexpr uint16_t TMA_REGISTER  = 0xFF06;
        static constexpr uint16_t TAC_REGISTER  = 0xFF07;

        static constexpr uint16_t IF_REGISTER   = 0xFF0F;

        static constexpr uint16_t LCDC_REGISTER = 0xFF40;
        static constexpr uint16_t STAT_REGISTER = 0xFF41;
        static constexpr uint16_t SCY_REGISTER  = 0xFF42;
        static constexpr uint16_t SCX_REGISTER  = 0xFF43;
        static constexpr uint16_t LY_REGISTER   = 0xFF44;
        static constexpr uint16_t LYC_REGISTER  = 0xFF45;
        static constexpr uint16_t DMA_REGISTER  = 0xFF46;
        static constexpr uint16_t BGP_REGISTER  = 0xFF47;
        static constexpr uint16_t OBP0_REGISTER = 0xFF48;
        static constexpr uint16_t OBP1_REGISTER = 0xFF49;
        static constexpr uint16_t WY_REGISTER   = 0xFF4A;
        static constexpr uint16_t WX_REGISTER   = 0xFF4B;

        static constexpr uint16_t IE_REGISTER   = 0xFFFF;

        uint8_t readIO(uint16_t address);
        void writeIO(uint16_t address, uint8_t value);

        uint8_t interruptStatus;
};

#endif // BUS_H
