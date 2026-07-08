// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <cstdint>
#include <string>
#include "StateReader.h"
#include "StateWriter.h"

class APU;
class Bus;
class Cartridge;
class CPU;
class Joypad;
class Memory;
class PPU;
class Timer;

class StateManager
{
    public:
        StateManager();
        virtual ~StateManager();

        void attachAPUInstance(APU* apu) { this->apu = apu; }
        void attachBusInstance(Bus* bus) { this->bus = bus; }
        void attachCartridgeInstance(Cartridge* cartridge) { this->cartridge = cartridge; }
        void attachCPUInstance(CPU* cpu) { this->cpu = cpu; }
        void attachJoypadInstance(Joypad* joypad) { this->joypad = joypad; }
        void attachMemoryInstance(Memory* memory) { this->memory = memory; }
        void attachPPUInstance(PPU* ppu) { this->ppu = ppu; }
        void attachTimerInstance(Timer* timer) { this->timer = timer; }

        bool save(const std::string& path);
        bool load(const std::string& path);

    protected:

    private:
        APU* apu;
        Bus* bus;
        Cartridge* cartridge;
        CPU* cpu;
        Joypad* joypad;
        Memory* memory;
        PPU* ppu;
        Timer* timer;

        static constexpr uint32_t kStateVersion = 1; // Save State file version
};

#endif // STATEMANAGER_H
