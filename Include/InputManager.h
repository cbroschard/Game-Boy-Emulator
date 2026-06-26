// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "Joypad.h"
#include "SDL3/SDL.h"

class InputManager
{
    public:
        InputManager();
        virtual ~InputManager();

        inline void attachJoypadInstance(Joypad* joypad) { this->joypad = joypad; }

        inline bool hasJoypad() const { return joypad ? 1 : 0; }

        void reset();

        void handleEvent(const SDL_Event& event);

    protected:

    private:
        Joypad* joypad;
};

#endif // INPUTMANAGER_H
