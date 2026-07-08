// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef UIBRIDGE_H
#define UIBRIDGE_H

#include <atomic>
#include <functional>
#include <string>
#include "EmulatorUI.h"

class UIBridge
{
    public:
        using VoidFn            = std::function<void()>;
        using StringFn          = std::function<void(const std::string&)>;
        using BoolFn            = std::function<bool()>;

        UIBridge(EmulatorUI& ui,
         std::atomic<bool>& uiPaused,
         std::atomic<bool>& running,
         StringFn saveState,
         StringFn loadState);

        virtual ~UIBridge();

        void processCommands();

    protected:

    private:
        EmulatorUI& ui_;

        std::atomic<bool>& uiPaused_;
        std::atomic<bool>& running_;

        StringFn saveState_;
        StringFn loadState_;
};

#endif // UIBRIDGE_H
