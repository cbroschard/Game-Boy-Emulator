// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/MLMonitorBackend.h"
#include "APU.h"
#include "Bus.h"
#include "Cartridge.h"
#include "EmulationSession.h"
#include "InputManager.h"
#include "Memory.h"
#include "PPU.h"
#include "Timer.h"

MLMonitorBackend::MLMonitorBackend() :
    apu(nullptr),
    bus(nullptr),
    cartridge(nullptr),
    cpu(nullptr),
    host(nullptr),
    inputMgr(nullptr),
    memory(nullptr),
    ppu(nullptr),
    timer(nullptr)
{

}

MLMonitorBackend::~MLMonitorBackend()
{

}

void MLMonitorBackend::enterMonitor()
{
    if (host)
        host->enterMonitor();
}
