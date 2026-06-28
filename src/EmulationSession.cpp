// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <stdexcept>
#include "EmulationSession.h"

EmulationSession::EmulationSession()
{
    wireUp();
    validateWiring();
}

EmulationSession::~EmulationSession()
{

}

void EmulationSession::reset()
{
    cpu.reset();
    memory.reset();
    cartridge.reset();
    bus.reset();
    apu.reset();
    ppu.reset();
    timer.reset();
    joypad.reset();
}

void EmulationSession::run()
{
    if (biosPath.empty())
        throw std::runtime_error("BIOS path has not been set.");

    if (cartridgePath.empty())
        throw std::runtime_error("Cartridge path has not been set.");

    if (!loadBIOS(biosPath))
        throw std::runtime_error("Failed to load BIOS: " + biosPath);

    if (!loadCartridge(cartridgePath))
        throw std::runtime_error("Failed to load Cartridge: " + cartridgePath);

    reset();

    if (!audioOutput.playAudio())
        throw std::runtime_error("Failed to start audio output.");

    constexpr int CPU_CLOCK_HZ = 4194304;

    constexpr int CYCLES_PER_SCANLINE = 456;
    constexpr int SCANLINES_PER_FRAME = 154;
    constexpr int CYCLES_PER_FRAME = CYCLES_PER_SCANLINE * SCANLINES_PER_FRAME;

    constexpr double FRAME_RATE =
        double(CPU_CLOCK_HZ) / double(CYCLES_PER_FRAME);

    constexpr double FRAME_TIME_SECONDS =
        1.0 / FRAME_RATE;

    const uint64_t performanceFrequency = SDL_GetPerformanceFrequency();

    uint64_t nextFrameTime = SDL_GetPerformanceCounter();

    bool running = true;

    while (running)
    {
        int frameCycles = 0;

        while (frameCycles < CYCLES_PER_FRAME && running)
        {
            SDL_Event event;

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                    break;
                }

                inputMgr.handleEvent(event);
            }

            if (!running)
                break;

            const int cpuCycles = cpu.step();

            if (cpuCycles <= 0)
                throw std::runtime_error("CPU step returned 0 or negative cycles.");

            apu.tick(cpuCycles);
            ppu.tick(cpuCycles);
            timer.tick(cpuCycles);

            frameCycles += cpuCycles;
        }

        if (!running)
            break;

        ppu.renderFrame(videoOutput);

        nextFrameTime += static_cast<uint64_t>(
            FRAME_TIME_SECONDS * double(performanceFrequency)
        );

        uint64_t now = SDL_GetPerformanceCounter();

        if (now < nextFrameTime)
        {
            uint64_t remainingTicks = nextFrameTime - now;

            double remainingMs =
                (double(remainingTicks) * 1000.0) /
                double(performanceFrequency);

            if (remainingMs > 1.0)
                SDL_Delay(static_cast<uint32_t>(remainingMs - 1.0));

            while (SDL_GetPerformanceCounter() < nextFrameTime)
            {
                // Small spin wait to improve frame pacing accuracy.
            }
        }
        else
        {
            // If emulation is running late, resync so lag does not accumulate forever.
            nextFrameTime = now;
        }
    }
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

    inputMgr.attachJoypadInstance(&joypad);

    ppu.attachBusInstance(&bus);

    timer.attachBusInstance(&bus);
}

void EmulationSession::validateWiring() const
{
    if (!cpu.hasBus())
        throw std::runtime_error("CPU bus is not attached.");

    if (!bus.hasMemory())
        throw std::runtime_error("Bus memory is not attached.");

    if (!bus.hasCartridge())
        throw std::runtime_error("Bus cartridge is not attached.");

    if (!bus.hasPPU())
        throw std::runtime_error("Bus PPU is not attached.");

    if (!bus.hasAPU())
        throw std::runtime_error("Bus APU is not attached.");

    if (!bus.hasJoypad())
        throw std::runtime_error("Bus joypad is not attached.");

    if (!bus.hasTimer())
        throw std::runtime_error("Bus timer is not attached.");

    if (!ppu.hasBus())
        throw std::runtime_error("PPU bus is not attached.");

    if (!apu.hasAudioOutput())
        throw std::runtime_error("APU audio output is not attached.");

    if (!inputMgr.hasJoypad())
        throw std::runtime_error("InputManager joypad is not attached.");

    if (!timer.hasBus())
        throw std::runtime_error("Timer bus is not attached.");
}
