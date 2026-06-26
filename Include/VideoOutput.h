// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#include <array>
#include <cstdint>
#include "SDL3/SDL.h"

class VideoOutput
{
    public:
        VideoOutput();
        virtual ~VideoOutput();

        static constexpr int SCREEN_WIDTH  = 160;
        static constexpr int SCREEN_HEIGHT = 144;
        static constexpr int SCALE = 3;

        void clear();
        void present();

        void renderFrame(const std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT>& pixels);

        void setPixel(int x, int y, uint8_t colorIndex);

    protected:

    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* texture;

        std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT> framebuffer;
};

#endif // VIDEOOUTPUT_H
