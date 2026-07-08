// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef EMULATIONSESSION_H
#define EMULATIONSESSION_H

#include <atomic>
#include <string>
#include "APU.h"
#include "AudioOutput.h"
#include "Bus.h"
#include "Cartridge.h"
#include "CPU.h"
#include "EmulatorUI.h"
#include "imgui/imgui_impl_sdl3.h"
#include "InputManager.h"
#include "Joypad.h"
#include "Memory.h"
#include "Debug/MLMonitor.h"
#include "Debug/MLMonitorBackend.h"
#include "MonitorController.h"
#include "SDL3/SDL.h"
#include "PPU.h"
#include "StateManager.h"
#include "Timer.h"
#include "UIBridge.h"
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
        EmulatorUI ui;
        InputManager inputManager;
        Joypad joypad;
        Memory memory;
        MLMonitor mlMonitor;
        MLMonitorBackend mlMonitorBackend;
        MonitorController monitorController;
        PPU ppu;
        StateManager stateManager;
        Timer timer;
        VideoOutput videoOutput;

        std::atomic<bool> uiPaused;
        std::atomic<bool> running;

        bool pendingSaveState;
        bool pendingLoadState;

        std::string pendingSavePath;
        std::string pendingLoadPath;

        UIBridge uiBridge;

        std::string biosPath;
        std::string cartridgePath;

        void wireUp();
        void validateWiring() const;

        inline bool loadBIOS(const std::string& path) { return memory.loadBIOS(path); }

        void renderUIFrame();

        bool processPendingStateCommands(uint64_t& nextFrameTime);
};

#endif // EMULATIONSESSION_H
