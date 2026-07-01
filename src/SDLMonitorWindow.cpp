// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "SDLMonitorWindow.h"
#include <algorithm>
#include <cstring>

// Color Definitions
static const SDL_Color COL_TEXT   = {160, 160, 255, 255}; // C64 Light Blue
static const SDL_Color COL_PROMPT = {100, 255, 100, 255}; // Green
static const SDL_Color COL_ERROR  = {255, 80,  80,  255}; // Red
static const SDL_Color COL_HEADER = {255, 255, 255, 255}; // White

static SDL_FRect toFRect(const SDL_Rect& r)
{
    return SDL_FRect{
        static_cast<float>(r.x),
        static_cast<float>(r.y),
        static_cast<float>(r.w),
        static_cast<float>(r.h)
    };
}

static const uint8_t font8x8_basic[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // !
    {0x66,0x66,0x22,0x00,0x00,0x00,0x00,0x00}, // "
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // #
    {0x08,0x1E,0x28,0x1C,0x0A,0x3C,0x08,0x00}, // $
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // %
    {0x1C,0x36,0x1C,0x3B,0x33,0x36,0x1C,0x00}, // &
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // '
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // (
    {0x18,0x30,0x60,0x60,0x60,0x30,0x18,0x00}, // )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // *
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // +
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // ,
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // -
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // .
    {0x00,0x60,0x30,0x18,0x0C,0x06,0x00,0x00}, // /
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // 0
    {0x18,0x18,0x38,0x18,0x18,0x18,0x3C,0x00}, // 1
    {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00}, // 2
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 3
    {0x06,0x0E,0x1E,0x66,0x7F,0x06,0x06,0x00}, // 4
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 5
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 6
    {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00}, // 7
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 8
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, // 9
    {0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00}, // :
    {0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30}, // ;
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // <
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // =
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00}, // >
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00}, // ?
    {0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00}, // @
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // A
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // B
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // C
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // D
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, // E
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, // F
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // G
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // H
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // I
    {0x1E,0x0C,0x0C,0x0C,0x0C,0xCC,0x78,0x00}, // J
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // K
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // L
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // M
    {0x66,0x76,0x7F,0x7F,0x6E,0x66,0x66,0x00}, // N
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // O
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // P
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // Q
    {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, // R
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // S
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // T
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // U
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // V
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // W
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // X
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // Y
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // Z
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // [
    {0x00,0x06,0x0C,0x18,0x30,0x60,0x00,0x00}, // Backslash
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // ]
    {0x18,0x3C,0x7E,0x18,0x18,0x18,0x18,0x00}, // ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00}, // _
    {0x18,0x0C,0x00,0x00,0x00,0x00,0x00,0x00}, // `
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // a
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // b
    {0x00,0x00,0x3C,0x60,0x60,0x60,0x3C,0x00}, // c
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // d
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // e
    {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00}, // f
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, // g
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // i
    {0x06,0x00,0x06,0x06,0x06,0x06,0x06,0x3C}, // j
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // l
    {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00}, // m
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // n
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // o
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // p
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, // q
    {0x00,0x00,0x5C,0x66,0x60,0x60,0x60,0x00}, // r
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // s
    {0x30,0x30,0x78,0x30,0x30,0x30,0x1C,0x00}, // t
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // u
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // v
    {0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00}, // w
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // x
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x0C,0x78}, // y
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // z
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // |
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // }
    {0x3B,0x6E,0x00,0x00,0x00,0x00,0x00,0x00}, // ~
    {0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x00}  // DEL (block)
};

SDLMonitorWindow::SDLMonitorWindow() :
    win(nullptr),
    ren(nullptr),
    fontTex(nullptr),
    width(900),
    height(550),
    charWidth(8),
    charHeight(8),
    lineHeight(10),
    padding(5),
    opened(false),
    historyIndex(0),
    scrollOffset(0),
    autoScroll(true),
    selecting(false),
    selAnchor(-1),
    selStart(-1),
    selEnd(-1),
    draggingThumb(false),
    thumbDragGrabY(0),
    cursorPos(0)
{

}

SDLMonitorWindow::~SDLMonitorWindow()
{
    close();
}

void SDLMonitorWindow::createFontTexture()
{
    if (!ren) return;

    // Create a surface first to manipulate pixels easily
    SDL_Surface* surf = SDL_CreateSurface(96 * 8, 8, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return;

    SDL_LockSurface(surf);
    uint32_t* pixels = (uint32_t*)surf->pixels;

    for (int c = 0; c < 96; ++c)
    {
        for (int y = 0; y < 8; ++y)
        {
            uint8_t row = font8x8_basic[c][y];
            for (int x = 0; x < 8; ++x)
            {
                // Check bit (most significant bit is left)
                bool pixelOn = (row >> (7 - x)) & 1;

                int surfX = c * 8 + x;
                int surfY = y;

                // White text, transparent background
                pixels[surfY * (surf->pitch / 4) + surfX] = pixelOn ? 0xFFFFFFFF : 0x00000000;
            }
        }
    }
    SDL_UnlockSurface(surf);

    // Convert to texture
    fontTex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_DestroySurface(surf);

    // Enable blending so transparency works
    SDL_SetTextureBlendMode(fontTex, SDL_BLENDMODE_BLEND);
}

bool SDLMonitorWindow::open(const char* title, int w, int h, ExecFn exec, PromptFn prompt)
{
    if (opened) return true;

    width = w;
    height = h;

    execFn = std::move(exec);
    promptFn = std::move(prompt);

    win = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!win) return false;

    SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    ren = SDL_CreateRenderer(win, nullptr);
    if (!ren)
    {
        SDL_DestroyWindow(win);
        win = nullptr;
        return false;
    }

    SDL_SetRenderVSync(ren, 1);

    createFontTexture();
    updateLayoutMetrics();

    opened = true;
    lines.clear();
    input.clear();
    cursorPos = 0;

    // Clear history or leave it for persistence?
    // Usually convenient to keep history, but for simplicity we can clear or keep.
    // Let's keep it but reset index.
    historyIndex = history.size();

    scrollOffset = 0;

    // Enables SDL_TEXTINPUT events for typing
    SDL_StartTextInput(win);

    appendLine("ML Monitor - type 'help' and press Enter", COL_HEADER);
    appendLine("------------------------------------------", COL_HEADER);

    return true;
}

void SDLMonitorWindow::close()
{
    if (!opened) return;

    if (win)
        SDL_StopTextInput(win);

    if (fontTex) SDL_DestroyTexture(fontTex);
    fontTex = nullptr;

    if (ren) SDL_DestroyRenderer(ren);
    ren = nullptr;

    if (win) SDL_DestroyWindow(win);
    win = nullptr;

    opened = false;
}

std::string SDLMonitorWindow::currentPrompt() const
{
    if (promptFn)
        return promptFn();

    return "> ";
}

void SDLMonitorWindow::appendLine(const std::string& s)
{
    appendLine(s, COL_TEXT);
}

void SDLMonitorWindow::appendLine(const std::string& s, SDL_Color color)
{
    lines.push_back({s, color});

    // If we're pinned to bottom, stay pinned. If user scrolled up, do not yank them down.
    if (scrollOffset == 0)
        autoScroll = true;

    if (autoScroll)
        scrollOffset = 0;
}

void SDLMonitorWindow::addChar(char c)
{
    if (c < 32 || c > 126)
        return;

    if (cursorPos < 0)
        cursorPos = 0;
    if (cursorPos > (int)input.size())
        cursorPos = (int)input.size();

    input.insert(input.begin() + cursorPos, c);
    cursorPos++;
}

void SDLMonitorWindow::backspace()
{
    if (cursorPos <= 0 || input.empty())
        return;

    input.erase(input.begin() + (cursorPos - 1));
    cursorPos--;
}

void SDLMonitorWindow::deleteChar()
{
    if (cursorPos < 0 || cursorPos >= (int)input.size())
        return;

    input.erase(input.begin() + cursorPos);
}

void SDLMonitorWindow::moveCursorLeft()
{
    if (cursorPos > 0)
        cursorPos--;
}

void SDLMonitorWindow::moveCursorRight()
{
    if (cursorPos < (int)input.size())
        cursorPos++;
}

void SDLMonitorWindow::moveCursorHome()
{
    cursorPos = 0;
}

void SDLMonitorWindow::moveCursorEnd()
{
    cursorPos = (int)input.size();
}

void SDLMonitorWindow::submitCommand()
{
    // Echo command in Green
    appendLine(currentPrompt() + input, COL_PROMPT);

    // Add to history if not empty
    if (!input.empty())
    {
        // Optional: don't add duplicates if same as last command
        if (history.empty() || history.back() != input)
        {
            history.push_back(input);
        }
    }

    historyIndex = history.size(); // point to new blank line at end

    std::string out;

    // IMPORTANT:
    // Even a blank line must be submitted, because interactive assembler mode
    // uses blank Enter to exit.
    if (execFn)
        out = execFn(input);

    // Split output into lines
    size_t start = 0;
    while (!out.empty() && start < out.size())
    {
        size_t nl = out.find('\n', start);
        std::string sub;

        if (nl == std::string::npos)
        {
            sub = out.substr(start);
            start = out.size();
        }
        else
        {
            sub = out.substr(start, nl - start);
            start = nl + 1;
        }

        // Simple heuristic for error coloring
        SDL_Color lineColor = COL_TEXT;
        if (sub.rfind("Error", 0) == 0 ||
            sub.rfind("Unable", 0) == 0 ||
            sub.rfind("Assembly error", 0) == 0)
        {
            lineColor = COL_ERROR;
        }

        appendLine(sub, lineColor);

        if (start >= out.size())
            break;
    }

    input.clear();
    scrollOffset = 0;
    cursorPos = 0;
}

void SDLMonitorWindow::handleEvent(const SDL_Event& e)
{
    if (!opened || !win) return;

    const Uint32 myId = SDL_GetWindowID(win);

    // Window events
    if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
        e.type == SDL_EVENT_WINDOW_RESIZED ||
        e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
        e.type == SDL_EVENT_WINDOW_MAXIMIZED)
    {
        if (e.window.windowID != myId)
            return;

        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        {
            close();
            return;
        }

        SDL_GetWindowSize(win, &width, &height);
        updateLayoutMetrics();

        scrollOffset = clampScrollOffset(scrollOffset);
        autoScroll   = (scrollOffset == 0);
        return;
    }

    // Filter events for THIS window only
    if ((e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) && e.key.windowID != myId) return;
    if (e.type == SDL_EVENT_TEXT_INPUT && e.text.windowID != myId) return;
    if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.windowID != myId) return;

    if ((e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        e.button.windowID != myId)
    {
        return;
    }

    if (e.type == SDL_EVENT_MOUSE_MOTION && e.motion.windowID != myId)
        return;

    // Text input
    if (e.type == SDL_EVENT_TEXT_INPUT)
    {
        for (const char* p = e.text.text; *p; ++p)
            addChar(*p);

        return;
    }

    // Key down
    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        SDL_Keymod mods = SDL_GetModState();
        bool ctrl = (mods & SDL_KMOD_CTRL) != 0;

        // Ctrl+V paste
        if (ctrl && e.key.key == SDLK_V)
        {
            char* clip = SDL_GetClipboardText();
            if (clip)
            {
                for (const char* p = clip; *p; ++p)
                {
                    char c = *p;
                    if (c == '\r') continue;

                    if (c == '\n')
                    {
                        submitCommand();
                        clearSelection();
                    }
                    else
                    {
                        addChar(c);
                    }
                }

                SDL_free(clip);
            }

            return;
        }

        // Ctrl+C copy
        if (ctrl && e.key.key == SDLK_C)
        {
            if (hasSelection())
            {
                std::string txt = getSelectedText();
                SDL_SetClipboardText(txt.c_str());
            }
            else
            {
                SDL_SetClipboardText(input.c_str());
            }

            return;
        }

        switch (e.key.key)
        {
            case SDLK_BACKSPACE:
                backspace();
                break;

            case SDLK_LEFT:
                moveCursorLeft();
                break;

            case SDLK_RIGHT:
                moveCursorRight();
                break;

            case SDLK_HOME:
                moveCursorHome();
                break;

            case SDLK_END:
                moveCursorEnd();
                break;

            case SDLK_DELETE:
                deleteChar();
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (hasSelection())
                {
                    std::string txt = getSelectedText();
                    SDL_SetClipboardText(txt.c_str());
                    clearSelection();
                }
                else
                {
                    submitCommand();
                    clearSelection();
                }
                break;

            case SDLK_ESCAPE:
                close();
                break;

            case SDLK_PAGEUP:
                scrollOffset -= 10;
                scrollOffset = clampScrollOffset(scrollOffset);
                autoScroll   = (scrollOffset == 0);
                break;

            case SDLK_PAGEDOWN:
                scrollOffset += 10;
                scrollOffset = clampScrollOffset(scrollOffset);
                autoScroll   = (scrollOffset == 0);
                break;

            case SDLK_UP:
                if (historyIndex > 0)
                {
                    historyIndex--;
                    input = history[historyIndex];
                    cursorPos = static_cast<int>(input.size());
                }
                break;

            case SDLK_DOWN:
                if (historyIndex < static_cast<int>(history.size()))
                {
                    historyIndex++;

                    if (historyIndex == static_cast<int>(history.size()))
                        input.clear();
                    else
                        input = history[historyIndex];

                    cursorPos = static_cast<int>(input.size());
                }
                break;

            default:
                break;
        }

        return;
    }

    // Mouse wheel
    if (e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        if (e.wheel.y > 0) scrollOffset -= 3;
        if (e.wheel.y < 0) scrollOffset += 3;

        scrollOffset = clampScrollOffset(scrollOffset);
        autoScroll   = (scrollOffset == 0);

        return;
    }

    // Mouse button down
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mouseX = static_cast<int>(e.button.x);
        int mouseY = static_cast<int>(e.button.y);

        SDL_Point p{ mouseX, mouseY };

        SDL_Rect thumb = getScrollbarThumbRect();
        if (SDL_PointInRect(&p, &thumb))
        {
            draggingThumb = true;
            thumbDragGrabY = mouseY - thumb.y;
            return;
        }

        SDL_Rect track = getScrollbarTrackRect();
        if (SDL_PointInRect(&p, &track))
        {
            setScrollFromThumbCenterY(mouseY);

            draggingThumb = true;
            SDL_Rect thumb2 = getScrollbarThumbRect();
            thumbDragGrabY = thumb2.h / 2;
            return;
        }

        int idx = lineIndexFromMouseY(mouseY);
        if (idx >= 0)
        {
            selecting = true;
            selAnchor = idx;
            selStart  = idx;
            selEnd    = idx;
        }
        else
        {
            clearSelection();
        }

        return;
    }

    // Mouse motion
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        int mouseY = static_cast<int>(e.motion.y);

        if (draggingThumb)
        {
            SDL_Rect thumb = getScrollbarThumbRect();
            int newThumbY = mouseY - thumbDragGrabY;
            int thumbCenterY = newThumbY + thumb.h / 2;

            setScrollFromThumbCenterY(thumbCenterY);
            return;
        }

        if (selecting)
        {
            int idx = lineIndexFromMouseY(mouseY);

            if (idx < 0 && selAnchor >= 0 && !lines.empty())
            {
                const int inputY = height - padding - lineHeight;
                const int historyBottomY = inputY - lineHeight;
                const int historyAreaBottom = inputY - 1;

                int first, last;
                visibleLineRange(first, last);

                if (mouseY < 0)
                    idx = first;
                else if (mouseY > historyAreaBottom)
                    idx = last;
                else
                    idx = (mouseY < historyBottomY / 2) ? first : last;
            }

            if (idx >= 0 && selAnchor >= 0)
            {
                selStart = std::min(selAnchor, idx);
                selEnd   = std::max(selAnchor, idx);
            }

            return;
        }

        return;
    }

    // Mouse button up
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT)
    {
        selecting = false;
        draggingThumb = false;
        return;
    }
}

void SDLMonitorWindow::drawString(int x, int y, const std::string& str, const SDL_Color& color)
{
    if (!fontTex) return;

    SDL_SetTextureColorMod(fontTex, color.r, color.g, color.b);

    SDL_FRect src{
        0.0f,
        0.0f,
        8.0f,
        8.0f
    };

    SDL_FRect dst{
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(charWidth),
        static_cast<float>(charHeight)
    };

    for (char c : str)
    {
        int index = static_cast<unsigned char>(c) - 32;
        if (index < 0 || index >= 96)
            index = 95;

        src.x = static_cast<float>(index * 8);

        SDL_RenderTexture(ren, fontTex, &src, &dst);

        dst.x += static_cast<float>(charWidth);
    }
}

void SDLMonitorWindow::render()
{
    if (!opened || !ren) return;

    // Background Color (Deep Dark Grey/Black)
    SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
    SDL_RenderClear(ren);

    // 1. Render Input Line at bottom
    int inputY = height - padding - lineHeight;

    std::string prompt = currentPrompt();
    drawString(padding, inputY, prompt, COL_PROMPT);
    drawString(padding + (int(prompt.length()) * charWidth), inputY, input, COL_TEXT);

    // Blinking cursor
    if ((SDL_GetTicks() / 500) % 2 == 0)
    {
        int cursorX = padding + (int(prompt.length() + cursorPos) * charWidth);
        SDL_Rect cursorRect = { cursorX, inputY, charWidth, charHeight };
        SDL_FRect fCursorRect = toFRect(cursorRect);

        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderFillRect(ren, &fCursorRect);
    }

    int historyBottomY = inputY - lineHeight;
    int historyCount = lines.size();

    // We iterate backwards from the end of the list minus scrollOffset
    int startIdx = historyCount - 1 - scrollOffset;

    int currentY = historyBottomY;

    for (int i = startIdx; i >= 0; --i)
    {
        if (currentY < 0) break; // Off top of screen

        bool selected = hasSelection() && (i >= selStart && i <= selEnd);
        if (selected)
        {
            SDL_Rect bg{ padding - 2, currentY - 1, width - (padding * 2) - 12, lineHeight };
            SDL_FRect fBg = toFRect(bg);

            SDL_SetRenderDrawColor(ren, 60, 60, 110, 255);
            SDL_RenderFillRect(ren, &fBg);
        }

        // Now passing the stored color for each line
        drawString(padding, currentY, lines[i].text, lines[i].color);
        currentY -= lineHeight;
    }

    // Scrollbar indicator (simple)
    int vis = visibleHistoryLines();
    int total = (int)lines.size();
    if (total > vis)
    {
        SDL_Rect track = getScrollbarTrackRect();
        SDL_Rect thumb = getScrollbarThumbRect();

        SDL_FRect fTrack = toFRect(track);
        SDL_FRect fThumb = toFRect(thumb);

        SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
        SDL_RenderFillRect(ren, &fTrack);

        SDL_SetRenderDrawColor(ren, 120, 120, 120, 255);
        SDL_RenderFillRect(ren, &fThumb);
    }

    SDL_RenderPresent(ren);
}

int SDLMonitorWindow::visibleHistoryLines() const
{
    int inputY = height - padding - lineHeight;
    int historyBottomY = inputY - lineHeight;

    int count = (historyBottomY / lineHeight) + 1;
    return std::max(0, count);
}

int SDLMonitorWindow::clampScrollOffset(int off) const
{
    int vis = visibleHistoryLines();
    int maxOff = std::max(0, (int)lines.size() - vis);
    if (off < 0) off = 0;
    if (off > maxOff) off = maxOff;
    return off;
}

void SDLMonitorWindow::clearSelection()
{
    selAnchor = selStart = selEnd = -1;
}

bool SDLMonitorWindow::hasSelection() const
{
    return (selStart >= 0 && selEnd >= 0 && selStart <= selEnd && selEnd < (int)lines.size());
}

std::string SDLMonitorWindow::getSelectedText() const
{
    if (!hasSelection()) return "";

    std::string out;
    for (int i = selStart; i <= selEnd; ++i)
    {
        out += lines[i].text;
        out += "\n";
    }
    return out;
}

int SDLMonitorWindow::lineIndexFromMouseY(int mouseY) const
{
    const int inputY = height - padding - lineHeight;

    // History occupies y = 0 .. inputY-1 (everything above the input line)
    const int historyAreaBottom = inputY - 1;

    // This is the y where the LAST visible history line starts (top of that line)
    const int historyBottomY = inputY - lineHeight;

    if (mouseY < 0 || mouseY > historyAreaBottom)
        return -1;

    if (mouseY > historyBottomY)
        mouseY = historyBottomY;

    const int rowFromBottom = (historyBottomY - mouseY) / lineHeight;

    const int historyCount = (int)lines.size();
    const int startIdx = historyCount - 1 - scrollOffset;

    const int idx = startIdx - rowFromBottom;
    if (idx < 0 || idx >= historyCount)
        return -1;

    return idx;
}

SDL_Rect SDLMonitorWindow::getScrollbarTrackRect() const
{
    int inputY = height - padding - lineHeight;
    int historyBottomY = inputY - lineHeight;

    SDL_Rect r;
    r.w = 10;
    r.x = width - r.w - 2;
    r.y = 2;
    r.h = std::max(0, historyBottomY - 2);
    return r;
}

SDL_Rect SDLMonitorWindow::getScrollbarThumbRect() const
{
    SDL_Rect track = getScrollbarTrackRect();

    int vis = visibleHistoryLines();
    int total = (int)lines.size();
    if (total <= vis || track.h <= 0)
        return SDL_Rect{track.x, track.y, track.w, track.h};

    float fracVisible = (float)vis / (float)total;
    int thumbH = std::max(16, (int)(track.h * fracVisible));

    int maxOff = std::max(1, total - vis);

    // Internal scrollOffset:
    //   0      = bottom/newest
    //   maxOff = top/oldest
    //
    // UI scrollbar:
    //   bottom of track = bottom/newest
    //   top of track    = top/oldest
    //
    // So invert the fraction.
    float fracScroll = 1.0f - ((float)scrollOffset / (float)maxOff);

    int travel = track.h - thumbH;
    int thumbY = track.y + (int)(fracScroll * travel);

    return SDL_Rect{track.x, thumbY, track.w, thumbH};
}

void SDLMonitorWindow::setScrollFromThumbCenterY(int thumbCenterY)
{
    SDL_Rect track = getScrollbarTrackRect();
    int vis = visibleHistoryLines();
    int total = (int)lines.size();
    if (total <= vis || track.h <= 0) return;

    int maxOff = std::max(1, total - vis);
    SDL_Rect thumb = getScrollbarThumbRect();
    int thumbH = thumb.h;

    int travel = track.h - thumbH;
    if (travel <= 0) return;

    int desiredThumbY = thumbCenterY - (thumbH / 2);
    desiredThumbY = std::clamp(desiredThumbY, track.y, track.y + travel);

    // UI:
    //   top of track    => oldest => maxOff
    //   bottom of track => newest => 0
    //
    // So invert the mapping.
    float frac = (float)(desiredThumbY - track.y) / (float)travel;
    scrollOffset = clampScrollOffset((int)((1.0f - frac) * maxOff + 0.5f));

    autoScroll = (scrollOffset == 0);
}

void SDLMonitorWindow::visibleLineRange(int& first, int& last) const
{
    const int historyCount = (int)lines.size();
    if (historyCount <= 0) { first = last = -1; return; }

    const int vis = visibleHistoryLines();

    // bottom-most visible line index (the one drawn at historyBottomY)
    last = historyCount - 1 - scrollOffset;
    last = std::clamp(last, 0, historyCount - 1);

    // top-most visible line index
    first = std::max(0, last - (vis - 1));
}

void SDLMonitorWindow::updateLayoutMetrics()
{
    // Base monitor design target
    const int targetCols = 80;
    const int targetRows = 32;

    // Compute integer scale factors so bitmap font stays sharp
    int scaleX = std::max(1, width / (targetCols * 8));
    int scaleY = std::max(1, height / (targetRows * 10));

    // Use the smaller scale to preserve aspect ratio
    int scale = std::max(1, std::min(scaleX, scaleY));

    scale = std::min(scale, 6);

    charWidth  = 8 * scale;
    charHeight = 8 * scale;

    // Extra spacing between lines
    lineHeight = charHeight + std::max(2, scale * 2);

    // Scale padding a little too
    padding = std::max(5, scale * 3);
}
