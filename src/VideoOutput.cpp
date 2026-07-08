// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <algorithm>
#include <stdexcept>
#include "VideoOutput.h"

VideoOutput::VideoOutput() :
    window(nullptr),
    renderer(nullptr),
    texture(nullptr)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(
            std::string("SDL Video Init Failed: ") + SDL_GetError()
        );
    }

    window = SDL_CreateWindow
    (
        "Game Boy Emulator",
        SCREEN_WIDTH * SCALE,
        SCREEN_HEIGHT * SCALE,
        SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        throw std::runtime_error(
            std::string("SDL_CreateWindow failed: ") + SDL_GetError()
        );
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    renderer = SDL_CreateRenderer(window, nullptr);

    if (!renderer)
    {
        throw std::runtime_error(
            std::string("SDL_CreateRenderer failed: ") + SDL_GetError()
        );
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    if (!texture)
    {
        throw std::runtime_error(
            std::string("SDL_CreateTexture failed: ") + SDL_GetError()
        );
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    io.FontGlobalScale = 2.0f;
    ImGui::GetStyle().ScaleAllSizes(2.0f);

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    framebuffer.fill(0xFFE0F8D0);
}

VideoOutput::~VideoOutput()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void VideoOutput::clear()
{
    framebuffer.fill(0xFFE0F8D0);

    if (!renderer)
        return;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void VideoOutput::present()
{
    beginFrame();
    renderGameFrame();
    endFrame();
}

void VideoOutput::beginFrame()
{
    if (!window || !renderer)
        return;

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void VideoOutput::renderGameFrame()
{
    if (!window || !renderer || !texture)
        return;

    SDL_UpdateTexture(
        texture,
        nullptr,
        framebuffer.data(),
        SCREEN_WIDTH * sizeof(uint32_t)
    );

    int windowWidth = 0;
    int windowHeight = 0;

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    const float scaleX =
        static_cast<float>(windowWidth) / static_cast<float>(SCREEN_WIDTH);

    const float scaleY =
        static_cast<float>(windowHeight) / static_cast<float>(SCREEN_HEIGHT);

    const float scale = std::min(scaleX, scaleY);

    const float outputWidth =
        static_cast<float>(SCREEN_WIDTH) * scale;

    const float outputHeight =
        static_cast<float>(SCREEN_HEIGHT) * scale;

    SDL_FRect dest;
    dest.x = (static_cast<float>(windowWidth) - outputWidth) * 0.5f;
    dest.y = (static_cast<float>(windowHeight) - outputHeight) * 0.5f;
    dest.w = outputWidth;
    dest.h = outputHeight;

    SDL_RenderTexture(renderer, texture, nullptr, &dest);
}

void VideoOutput::endFrame()
{
    if (!renderer)
        return;

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
}

void VideoOutput::renderFrame(const std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT>& pixels)
{
    framebuffer = pixels;
    renderGameFrame();
}

void VideoOutput::setPixel(int x, int y, uint8_t colorIndex)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
        return;

    static constexpr uint32_t palette[4] =
    {
        0xFFE0F8D0, // lightest
        0xFF88C070,
        0xFF346856,
        0xFF081820  // darkest
    };

    framebuffer[y * SCREEN_WIDTH + x] = palette[colorIndex & 0x03];
}
