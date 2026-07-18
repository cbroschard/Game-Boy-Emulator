// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include "EmulationSession.h"

EmulationSession::EmulationSession() :
    hardwareMode(HardwareMode::DMG),
    uiPaused(false),
    running(true),
    pendingSaveState(false),
    pendingLoadState(false),
    skipNextBreakpointCheck(false),
    uiBridge
    (
        ui,
        uiPaused,
        running,
        [this](const std::string& path)
        {
            pendingSaveState = true;
            pendingSavePath = path;
        },
        [this](const std::string& path)
        {
            pendingLoadState = true;
            pendingLoadPath = path;
        }
    )
{
    wireUp();
    validateWiring();
}

EmulationSession::~EmulationSession() = default;

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
    if (dmgBIOSPath.empty())
        throw std::runtime_error("DMG BIOS path has not been set.");

    if (cgbBIOSPath.empty())
        throw std::runtime_error{"CGB BIOS path has not been set."};

    if (cartridgePath.empty())
        throw std::runtime_error("Cartridge path has not been set.");

    if (!loadCartridge(cartridgePath))
        throw std::runtime_error("Failed to load Cartridge: " + cartridgePath);

    hardwareMode =
    supportsCGB(cartridge.getColorSupport())
        ? HardwareMode::CGB
        : HardwareMode::DMG;


    // Notify subsystems of the hardware mode
    ppu.setHardwareMode(hardwareMode);
    memory.setHardwareMode(hardwareMode);

    if (hardwareMode == HardwareMode::DMG)
    {
        if (!loadBIOS(dmgBIOSPath))
            throw std::runtime_error("Failed to load DMG BIOS: " + dmgBIOSPath);
    }
    else
    {
        if (!loadBIOS(cgbBIOSPath))
            throw std::runtime_error("Failed to load CGB BIOS: " + cgbBIOSPath);
    }

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

    running = true;
    uiPaused = false;

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F12)
            {
                monitorController.toggleMonitor();
                continue;
            }

            ImGuiIO& io = ImGui::GetIO();

            if (io.WantCaptureKeyboard || io.WantCaptureMouse)
                continue;

            if (monitorController.handleEvent(event))
                continue;

            inputManager.handleEvent(event);
        }

        if (!running)
            break;

        uiPaused = ui.isDialogOpen() || pendingSaveState || pendingLoadState;

        const bool emulationPaused =
            uiPaused || monitorController.isOpen();

        if (monitorController.isOpen())
        {
            monitorController.tick();
        }

        if (!emulationPaused)
        {
            int frameCycles = 0;

            while (!ppu.isFrameReady() &&
                   frameCycles < CYCLES_PER_FRAME &&
                   running)
            {

                const uint16_t pc = cpu.getPC();

                if (skipNextBreakpointCheck)
                {
                        skipNextBreakpointCheck = false;
                }
                else if (mlMonitor.hasBreakpoint(pc))
                {
                    std::ostringstream message;

                    message
                        << "Breakpoint hit at $"
                        << std::uppercase
                        << std::hex
                        << std::setw(4)
                        << std::setfill('0')
                        << pc;

                    mlMonitor.queueAsyncLine(message.str());
                    skipNextBreakpointCheck = true;

                    monitorController.openMonitor();
                    break;
                }

                const int cpuCycles = cpu.step();

                if (cpuCycles <= 0)
                    throw std::runtime_error("CPU step returned 0 or negative cycles.");

                frameCycles += cpuCycles;

                apu.tick(cpuCycles);
                ppu.tick(cpuCycles);
                timer.tick(cpuCycles);
            }
        }

        if (!running)
            break;

        renderUIFrame();

        if (ppu.isFrameReady())
            ppu.clearFrameReady();

        uiBridge.processCommands();

        const bool didStateCommand =
            processPendingStateCommands(nextFrameTime);

        if (didStateCommand)
        {
            renderUIFrame();
        }

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
    bus.attachMLMonitorInstance(&mlMonitor);
    bus.attachPPUInstance(&ppu);
    bus.attachTimerInstance(&timer);

    cpu.attachBusInstance(&bus);

    inputManager.attachJoypadInstance(&joypad);

    mlMonitor.attachMLMonitorBackendInstance(&mlMonitorBackend);

    mlMonitorBackend.attachAPUInstance(&apu);
    mlMonitorBackend.attachBusInstance(&bus);
    mlMonitorBackend.attachCartridgeInstance(&cartridge);
    mlMonitorBackend.attachCPUInstance(&cpu);
    mlMonitorBackend.attachEmulationSessionInstance(this);
    mlMonitorBackend.attachInputManagerInstance(&inputManager);
    mlMonitorBackend.attachMemoryInstance(&memory);
    mlMonitorBackend.attachPPUInstance(&ppu);
    mlMonitorBackend.attachTimerInstance(&timer);

    monitorController.attachMLMonitorInstance(&mlMonitor);

    ppu.attachBusInstance(&bus);

    stateManager.attachAPUInstance(&apu);
    stateManager.attachBusInstance(&bus);
    stateManager.attachCartridgeInstance(&cartridge);
    stateManager.attachCPUInstance(&cpu);
    stateManager.attachJoypadInstance(&joypad);
    stateManager.attachMemoryInstance(&memory);
    stateManager.attachPPUInstance(&ppu);
    stateManager.attachTimerInstance(&timer);

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

    if (!inputManager.hasJoypad())
        throw std::runtime_error("InputManager joypad is not attached.");

    if (!timer.hasBus())
        throw std::runtime_error("Timer bus is not attached.");
}

void EmulationSession::renderUIFrame()
{
    videoOutput.beginFrame();

    ppu.presentFrame(videoOutput);
    ui.draw();

    videoOutput.endFrame();
}

bool EmulationSession::processPendingStateCommands(uint64_t& nextFrameTime)
{
    if (!pendingSaveState && !pendingLoadState)
        return false;

    uiPaused = true;

    bool didWork = false;

    if (pendingSaveState)
    {
        pendingSaveState = false;

        stateManager.save(pendingSavePath);

        pendingSavePath.clear();
        didWork = true;
    }

    if (pendingLoadState)
    {
        pendingLoadState = false;

        const bool ok = stateManager.load(pendingLoadPath);

        pendingLoadPath.clear();

        ppu.clearFrameReady();

        if (!ok)
            std::cout << "Load state failed\n";

        didWork = true;
    }

    nextFrameTime = SDL_GetPerformanceCounter();

    uiPaused = false;

    return didWork;
}
