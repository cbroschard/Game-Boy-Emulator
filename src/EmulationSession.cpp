// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "EmulationSession.h"

EmulationSession::EmulationSession()
{

}

EmulationSession::~EmulationSession()
{

}

void EmulationSession::wireUp()
{
    apu.attachAudioOutputInstance(&audioOutput);

    bus.attachAPUInstance(&apu);
    bus.attachCartridgeInstance(&cartridge);
    bus.attachJoypadInstance(&joypad);
    bus.attachMemoryInstance(&memory);
    bus.attachPPUInstance(&ppu);
    bus.attachTimerInstance(&timer);

    cpu.attachBusInstance(&bus);

    ppu.attachVideoOutputInstance(&videoOutput);
}
