// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "InputManager.h"

InputManager::InputManager() :
    joypad(nullptr)
{

}

InputManager::~InputManager() = default;

void InputManager::reset()
{
    if (joypad)
        joypad->reset();
}

void InputManager::handleEvent(const SDL_Event& event)
{
    if (event.type != SDL_EVENT_KEY_DOWN &&
        event.type != SDL_EVENT_KEY_UP)
    {
        return;
    }

    const bool pressed = event.type == SDL_EVENT_KEY_DOWN;
    const SDL_Keycode key = event.key.key;

    switch (key)
    {
        case SDLK_Z:
            joypad->setButtonA(pressed);
            break;

        case SDLK_X:
            joypad->setButtonB(pressed);
            break;

        case SDLK_RETURN:
            joypad->setStart(pressed);
            break;

        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
            joypad->setSelect(pressed);
            break;

        case SDLK_RIGHT:
            joypad->setRight(pressed);
            break;

        case SDLK_LEFT:
            joypad->setLeft(pressed);
            break;

        case SDLK_UP:
            joypad->setUp(pressed);
            break;

        case SDLK_DOWN:
            joypad->setDown(pressed);
            break;

        default:
            break;
    }
}
