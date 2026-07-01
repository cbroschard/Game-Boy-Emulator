// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "MonitorController.h"

MonitorController::MonitorController() :
    mlMonitor(nullptr)
{
    sdlMonitorWindow = std::make_unique<SDLMonitorWindow>();
}

MonitorController::~MonitorController()
{
    try
    {
        closeMonitor();
    }
    catch(...)
    {

    }
}

void MonitorController::openMonitor()
{
    ensureWindow();

    if (mlMonitor) mlMonitor->setRunningFlag(true);

    if (!sdlMonitorWindow->isOpen())
    {
        sdlMonitorWindow->open("ML Monitor", 900, 550,
            [this](const std::string& cmd) -> std::string
            {
                if (!mlMonitor) return "Monitor not available\n";
                return mlMonitor->executeAndCapture(cmd);
            },
            [this]() -> std::string
            {
                if (!mlMonitor) return "> ";
                return mlMonitor->getPrompt();
            });
    }

    // Show anything queued before the UI opened (watchpoints/breakpoints/etc.)
    drainAsyncLines();
}

void MonitorController::closeMonitor()
{
    if (sdlMonitorWindow && sdlMonitorWindow->isOpen())
        sdlMonitorWindow->close();

    if (mlMonitor)
        mlMonitor->setRunningFlag(false);
}

void MonitorController::toggleMonitor()
{
    if (isOpen()) closeMonitor();
    else openMonitor();
}

bool MonitorController::handleEvent(const SDL_Event& ev)
{
    if (!isOpen())
        return false;

    sdlMonitorWindow->handleEvent(ev);

    // If the SDL monitor window was closed by X/Escape.
    if (!isOpen())
    {
        if (mlMonitor)
            mlMonitor->setRunningFlag(false);

        return true;
    }

    // If a monitor command requested exit/q/quit.
    if (mlMonitor && !mlMonitor->getRunningFlag())
    {
        sdlMonitorWindow->close();
        return true;
    }

    // While monitor is open, swallow keyboard/text/mouse/window events.
    if (ev.type == SDL_EVENT_TEXT_INPUT ||
        ev.type == SDL_EVENT_TEXT_EDITING ||
        ev.type == SDL_EVENT_KEY_DOWN ||
        ev.type == SDL_EVENT_KEY_UP ||
        ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        ev.type == SDL_EVENT_MOUSE_BUTTON_UP ||
        ev.type == SDL_EVENT_MOUSE_MOTION ||
        ev.type == SDL_EVENT_MOUSE_WHEEL ||
        ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
        ev.type == SDL_EVENT_WINDOW_RESIZED ||
        ev.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
        ev.type == SDL_EVENT_WINDOW_MAXIMIZED)
    {
        return true;
    }

    return false;
}

void MonitorController::tick()
{
    if (!isOpen())
        return;

    // Close if monitor requested exit.
    if (mlMonitor && !mlMonitor->getRunningFlag())
    {
        sdlMonitorWindow->close();
        return;
    }

    drainAsyncLines();
    sdlMonitorWindow->render();
}

void MonitorController::ensureWindow()
{
    if (!sdlMonitorWindow)
        sdlMonitorWindow = std::make_unique<SDLMonitorWindow>();
}

void MonitorController::drainAsyncLines()
{
    if (!mlMonitor || !sdlMonitorWindow || !sdlMonitorWindow->isOpen())
        return;

    for (const auto& line : mlMonitor->drainAsyncLines())
        sdlMonitorWindow->appendLine(line);
}

void MonitorController::appendLine(const std::string& line)
{
    if (sdlMonitorWindow && sdlMonitorWindow->isOpen())
        sdlMonitorWindow->appendLine(line);
}
