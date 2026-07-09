// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MLMONITORBACKEND_H
#define MLMONITORBACKEND_H

#include <cstdint>
#include "Bus.h"
#include "CPU.h"

class APU;
class Cartridge;
class EmulationSession;
class InputManager;
class Memory;
class PPU;
class Timer;

class MLMonitorBackend
{
    public:
        MLMonitorBackend();
        virtual ~MLMonitorBackend();

        inline void attachAPUInstance(APU* apu) { this->apu = apu; }
        inline void attachBusInstance(Bus* bus) { this->bus = bus; }
        inline void attachCartridgeInstance(Cartridge* cartridge) { this->cartridge = cartridge; }
        inline void attachCPUInstance(CPU* cpu) { this->cpu = cpu; }
        inline void attachEmulationSessionInstance(EmulationSession* host) { this->host = host; }
        inline void attachInputManagerInstance(InputManager* inputManager) { this->inputManager = inputManager; }
        inline void attachMemoryInstance(Memory* memory) { this->memory = memory; }
        inline void attachPPUInstance(PPU* ppu) { this->ppu = ppu; }
        inline void attachTimerInstance(Timer* timer) { this->timer = timer; }

        void enterMonitor();

        // Bus
        inline uint8_t readRAM(uint16_t address) const { return bus->read(address); }
        inline void writeRAM(uint16_t address, uint8_t value) { bus->write(address, value); }

        // CPU
        inline CPU& getCPU() { return *cpu; }
        inline const CPU& getCPU() const { return *cpu; }
        inline uint16_t getPC() const { return cpu->getPC(); }
        inline void setPC(uint16_t address) { cpu->setPC(address); }

        void printCPUCycles() const;
        void printCPUFlags() const;
        void printCPUIRQState() const;
        void printCPUStack(int count) const;
        void printCPUState() const;

    protected:

    private:
        // Non-owning pointers
        APU* apu;
        Bus* bus;
        Cartridge* cartridge;
        CPU* cpu;
        EmulationSession* host;
        InputManager* inputManager;
        Memory* memory;
        PPU* ppu;
        Timer* timer;

};

#endif // MLMONITORBACKEND_H
