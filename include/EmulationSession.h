// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef EMULATIONSESSION_H
#define EMULATIONSESSION_H

#include <string>
#include "APU.h"
#include "AudioOutput.h"
#include "Bus.h"
#include "Cartridge.h"
#include "CPU.h"
#include "InputManager.h"
#include "Joypad.h"
#include "Memory.h"
#include "Debug/MLMonitor.h"
#include "Debug/MLMonitorBackend.h"
#include "MonitorController.h"
#include "SDL3/SDL.h"
#include "PPU.h"
#include "Timer.h"
#include "VideoOutput.h"

class EmulationSession
{
    public:
        EmulationSession();
        virtual ~EmulationSession();

        void reset();

        void run();

        inline void setBIOSPath(const std::string& path) { biosPath = path; }
        inline void setCartridgePath(const std::string &path) { cartridgePath = path; }

        inline bool loadCartridge(const std::string& path) { return cartridge.loadCartridge(path); }

        // ML Monitor
        inline void enterMonitor() { monitorController.openMonitor(); }

    protected:

    private:
        APU apu;
        AudioOutput audioOutput;
        Bus bus;
        Cartridge cartridge;
        CPU cpu;
        InputManager inputMgr;
        Joypad joypad;
        Memory memory;
        MLMonitor mlMonitor;
        MLMonitorBackend mlmonitorBackend;
        MonitorController monitorController;
        PPU ppu;
        Timer timer;
        VideoOutput videoOutput;

        std::string biosPath;
        std::string cartridgePath;

        void wireUp();
        void validateWiring() const;

        inline bool loadBIOS(const std::string& path) { return memory.loadBIOS(path); }
};

#endif // EMULATIONSESSION_H
