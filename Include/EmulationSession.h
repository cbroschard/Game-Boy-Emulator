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
#include "Joypad.h"
#include "memory.h"
#include "PPU.h"
#include "Timer.h"
#include "VideoOutput.h"

class EmulationSession
{
    public:
        EmulationSession();
        virtual ~EmulationSession();

        void run();

        void setBIOSPath(const std::string& path);

        void loadCartridge(const std::string& path);

    protected:

    private:
        APU apu;
        AudioOutput audioOutput;
        Bus bus;
        Cartridge cartridge;
        CPU cpu;
        Joypad joypad;
        Memory memory;
        PPU ppu;
        Timer timer;
        VideoOutput videoOutput;

        void wireUp();
};

#endif // EMULATIONSESSION_H
