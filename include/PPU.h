// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef PPU_H
#define PPU_H

#include <array>
#include <cstdint>
#include "StateReader.h"
#include "StateWriter.h"
#include "VideoOutput.h"

class Bus;

class PPU
{
    public:
        PPU();
        virtual ~PPU();

        inline void attachBusInstance(Bus* bus) { this->bus = bus; }

        void reset();

        void tick(int cyclesElapsed);

        void saveState(StateWriter& wrtr) const;
        bool loadState(const StateReader::Chunk& chunk, StateReader& rdr);

        inline bool hasBus() const { return bus ? 1 : 0; }

        inline uint8_t readOAM(uint16_t offset) const { return oam[offset]; }
        inline void writeOAM(uint16_t offset, uint8_t value) { oam[offset] = value; }

        inline uint8_t readVRAM(uint16_t offset) const { return vram[offset]; }
        inline void writeVRAM(uint16_t offset, uint8_t value) { vram[offset] = value; }

        uint8_t readRegister(uint16_t address) const;
        void writeRegister(uint16_t address, uint8_t value);

        inline bool isFrameReady() const { return frameReady; }
        inline void clearFrameReady() { frameReady = false; }

        inline void presentFrame(VideoOutput& videoOutput) { videoOutput.renderFrame(framebuffer); }

    protected:

    private:
        Bus* bus;

        std::array<uint8_t, 0x2000> vram{}; // 8 KB, $8000-$9FFF
        std::array<uint8_t, 0xA0>   oam{};  // 160 bytes, $FE00-$FE9F

        std::array<uint32_t, 160 * 144> framebuffer;

        enum class PPUMode : uint8_t
        {
            HBlank  = 0,
            VBlank  = 1,
            OAM     = 2,
            Drawing = 3
        };

        struct SpriteEntry
        {
            uint8_t index;
            uint8_t y;
            uint8_t x;
            uint8_t tile;
            uint8_t attributes;
        };

        PPUMode mode;

        bool frameReady;
        bool scanLineRendered;

        uint8_t stat;
        uint8_t lcdc;
        uint8_t scy;
        uint8_t scx;
        uint8_t ly;
        uint8_t lyc;

        uint8_t bgp;

        uint8_t obp0;
        uint8_t obp1;

        uint8_t wy;
        uint8_t wx;

        uint16_t dots;
        uint8_t windowLineCounter;

        inline bool isLCDEnabled() const            { return lcdc & 0x80; }
        inline bool useWindowTileMapHigh() const    { return lcdc & 0x40; }
        inline bool isWindowEnabled() const         { return lcdc & 0x20; }
        inline bool useTileDataUnsigned() const     { return lcdc & 0x10; }
        inline bool useBGTileMapHigh() const        { return lcdc & 0x08; }
        inline bool useOBJ8x16() const              { return lcdc & 0x04; }
        inline bool isOBJEnabled() const            { return lcdc & 0x02; }
        inline bool isBGWindowEnabled() const       { return lcdc & 0x01; }

        void setMode(PPUMode mode);

        void requestLCDStatInterrupt();
        void requestVBlankInterrupt();

        void updateLYCCompare();

        void renderScanline(uint8_t line);

        uint8_t fetchBGPixel(int x, int y);
        uint8_t fetchWindowPixel(int x);

        uint32_t dmgShadeToRGB(uint8_t shade);

        inline bool areSpritesEnabled() const { return (lcdc & 0x02) != 0; }
        inline uint8_t getSpriteHeight() const { return (lcdc & 0x04) ? 16 : 8; }

        void renderSpritesOnScanline(uint8_t line, const uint8_t bgColorIds[160]);
        void drawSpriteLine(const SpriteEntry& sprite, uint8_t line, const uint8_t bgColorIds[160]);
};

#endif // PPU_H
