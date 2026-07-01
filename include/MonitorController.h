// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MONITORCONTROLLER_H
#define MONITORCONTROLLER_H

#include <string>
#include "Debug/MLMonitor.h"
#include "SDL3/SDL.h"
#include "SDLMonitorWindow.h"

class MonitorController
{
    public:
        MonitorController();
        virtual ~MonitorController();

        inline void attachMLMonitorInstance(MLMonitor* mlMonitor) { this->mlMonitor = mlMonitor; }

        inline bool isOpen() const { return sdlMonitorWindow->isOpen(); }

        void openMonitor();     // ensure open + pause
        void closeMonitor();    // close + resume if we paused
        void toggleMonitor();   // open if closed, close if open

        bool handleEvent(const SDL_Event& ev);

        void tick();

        void appendLine(const std::string& line);

    protected:

    private:
        std::unique_ptr<SDLMonitorWindow> sdlMonitorWindow;

        // Non-owning pointers
        MLMonitor* mlMonitor;

        void ensureWindow();
        void drainAsyncLines();
};

#endif // MONITORCONTROLLER_H
