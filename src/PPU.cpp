// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Bus.h"
#include "PPU.h"
#include "VideoOutput.h"

PPU::PPU() :
    bus(nullptr),
    stat(0x80),
    lcdc(0x91),
    scy(0),
    scx(0),
    ly(0),
    lyc(0),
    bgp(0xFC),
    obp0(0xFF),
    obp1(0xFF),
    wy(0),
    wx(0),
    dots(0)
{
    mode = PPUMode::OAM;
}

PPU::~PPU()
{

}

void PPU::reset()
{
    oam.fill(0x00);
    vram.fill(0x00);

    framebuffer.fill(0);

    mode            = PPUMode::OAM;
    stat            = 0x80 | static_cast<uint8_t>(mode);

    lcdc            = 0x91;

    scy             = 0;
    scx             = 0;
    ly              = 0;
    lyc             = 0;

    bgp             = 0xFC;
    obp0            = 0xFF;
    obp1            = 0xFF;

    wy              = 0;
    wx              = 0;
    dots            = 0;

    updateLYCCompare();
}

void PPU::tick(int cyclesElapsed)
{
    if (!isLCDEnabled())
    {
        dots = 0;
        ly = 0;
        mode = PPUMode::HBlank;
        stat = (stat & 0xFC) | static_cast<uint8_t>(PPUMode::HBlank);
        updateLYCCompare();
        return;
    }

    dots += cyclesElapsed;

    while (dots >= 456)
    {
        dots -= 456;
        ly++;

        updateLYCCompare();

        if (ly == 144)
        {
            setMode(PPUMode::VBlank);
            requestVBlankInterrupt();
        }
        else if (ly > 153)
        {
            ly = 0;
            updateLYCCompare();
            setMode(PPUMode::OAM);
        }
    }

    if (ly >= 144)
    {
        setMode(PPUMode::VBlank);
    }
    else
    {
        if (dots < 80)
            setMode(PPUMode::OAM);
        else if (dots < 252)
            setMode(PPUMode::Drawing);
        else
            setMode(PPUMode::HBlank);
    }
}

uint8_t PPU::readRegister(uint16_t address) const
{
    switch (address)
    {
        case 0xFF40:
            return lcdc;

        case 0xFF41:
            return stat | 0x80;

        case 0xFF42:
            return scy;

        case 0xFF43:
            return scx;

        case 0xFF44:
            return ly;

        case 0xFF45:
            return lyc;

        case 0xFF47:
            return bgp;

        case 0xFF48:
            return obp0;

        case 0xFF49:
            return obp1;

        case 0xFF4A:
            return wy;

        case 0xFF4B:
            return wx;

        default:
            return 0xFF;
    }
}

void PPU::writeRegister(uint16_t address, uint8_t value)
{
    switch (address)
    {
        case 0xFF40:
        {
            const bool wasEnabled = isLCDEnabled();

            lcdc = value;

            const bool nowEnabled = isLCDEnabled();

            if (wasEnabled && !nowEnabled)
            {
                dots = 0;
                ly = 0;
                mode = PPUMode::HBlank;
                stat = (stat & 0xFC) | static_cast<uint8_t>(PPUMode::HBlank);
                updateLYCCompare();
            }

            return;
        }

        case 0xFF41:
        {
            stat = (stat & 0x87) | (value & 0x78);
            return;
        }

        case 0xFF42:
        {
            scy = value;
            return;
        }

        case 0xFF43:
        {
            scx = value;
            return;
        }

        case 0xFF44:
        {
            return;
        }

        case 0xFF45:
        {
            lyc = value;
            updateLYCCompare();
            return;
        }

        case 0xFF47:
        {
            bgp = value;
            return;
        }

        case 0xFF48:
        {
            obp0 = value;
            return;
        }

        case 0xFF49:
        {
            obp1 = value;
            return;
        }

        case 0xFF4A:
        {
            wy = value;
            return;
        }

        case 0xFF4B:
        {
            wx = value;
            return;
        }

        default:
            return;
    }
}

void PPU::renderFrame(VideoOutput& videoOutput)
{
    if (!isLCDEnabled())
    {
        framebuffer.fill(0xFFFFFFFF);
        videoOutput.renderFrame(framebuffer);
        return;
    }

    for (uint8_t line = 0; line < 144; line++)
        renderScanline(line);

    videoOutput.renderFrame(framebuffer);
}

void PPU::setMode(PPUMode newMode)
{
    if (mode == newMode)
        return;

    mode = newMode;

    // Update STAT mode bits.
    stat = (stat & 0xFC) | static_cast<uint8_t>(newMode);

    // Request STAT interrupt if enabled for this mode.
    switch (newMode)
    {
        case PPUMode::HBlank:
        {
            if (stat & 0x08) // STAT bit 3: HBlank interrupt enable
                requestLCDStatInterrupt();
            break;
        }

        case PPUMode::VBlank:
        {
            if (stat & 0x10) // STAT bit 4: VBlank STAT interrupt enable
                requestLCDStatInterrupt();
            break;
        }

        case PPUMode::OAM:
        {
            if (stat & 0x20) // STAT bit 5: OAM interrupt enable
                requestLCDStatInterrupt();
            break;
        }

        case PPUMode::Drawing:
            // Mode 3 no STAT interrupt source.
            break;
    }
}

void PPU::requestLCDStatInterrupt()
{
    if (bus)
        bus->requestInterrupt(Bus::Interrupt::LCDStat);
}

void PPU::requestVBlankInterrupt()
{
    if (bus)
        bus->requestInterrupt(Bus::Interrupt::VBlank);
}

void PPU::updateLYCCompare()
{
    if (ly == lyc)
    {
        const bool wasEqual = (stat & 0x04) != 0;

        stat |= 0x04;

        if (!wasEqual && (stat & 0x40))
        {
            requestLCDStatInterrupt();
        }
    }
    else
    {
        stat &= static_cast<uint8_t>(~0x04);
    }
}

void PPU::renderScanline(uint8_t line)
{
    if (line >= 144)
        return;

    uint8_t bgColorIds[160];

    for (uint16_t x = 0; x < 160; x++)
    {
        uint8_t colorId = 0;

        if (isBGWindowEnabled())
            colorId = fetchBGPixel(x, line);

        int windowLeft = int(wx) - 7;

        if (isWindowEnabled() &&
            isBGWindowEnabled() &&
            line >= wy &&
            int(x) >= windowLeft)
        {
            colorId = fetchWindowPixel(x, line);
        }

        bgColorIds[x] = colorId;

        uint8_t shade = (bgp >> (colorId * 2)) & 0x03;
        framebuffer[line * 160 + x] = dmgShadeToRGB(shade);
    }

    if (areSpritesEnabled())
        renderSpritesOnScanline(line, bgColorIds);
}

uint8_t PPU::fetchBGPixel(int x, int y)
{
    if (!isBGWindowEnabled())
        return 0;

    uint8_t bgX = static_cast<uint8_t>(scx + x);
    uint8_t bgY = static_cast<uint8_t>(scy + y);

    uint8_t tileX = bgX / 8;
    uint8_t tileY = bgY / 8;

    uint8_t pixelX = bgX % 8;
    uint8_t pixelY = bgY % 8;

    uint16_t tileMapBase = useBGTileMapHigh() ? 0x1C00 : 0x1800;
    uint16_t tileMapOffset = tileMapBase + tileY * 32 + tileX;

    uint8_t tileNumber = vram[tileMapOffset];

    uint16_t tileDataOffset;

    if (useTileDataUnsigned())
    {
        tileDataOffset = static_cast<uint16_t>(tileNumber) * 16;
    }
    else
    {
        int8_t signedTile = static_cast<int8_t>(tileNumber);
        tileDataOffset = static_cast<uint16_t>(0x1000 + signedTile * 16);
    }

    uint16_t rowOffset = tileDataOffset + pixelY * 2;

    uint8_t lo = vram[rowOffset];
    uint8_t hi = vram[rowOffset + 1];

    uint8_t bit = 7 - pixelX;

    uint8_t lowBit = (lo >> bit) & 0x01;
    uint8_t highBit = (hi >> bit) & 0x01;

    return lowBit | (highBit << 1);
}

void PPU::drawSpriteLine(const SpriteEntry& sprite, uint8_t line, const uint8_t bgColorIds[160])
{
    const uint8_t spriteHeight = getSpriteHeight();

    int screenX = int(sprite.x) - 8;
    int screenY = int(sprite.y) - 16;

    bool priorityBehindBG = (sprite.attributes & 0x80) != 0;
    bool yFlip            = (sprite.attributes & 0x40) != 0;
    bool xFlip            = (sprite.attributes & 0x20) != 0;
    bool useOBP1          = (sprite.attributes & 0x10) != 0;

    int row = int(line) - screenY;

    if (yFlip)
        row = spriteHeight - 1 - row;

    uint8_t tileNumber = sprite.tile;

    // In 8x16 mode, bit 0 of tile number is ignored.
    if (spriteHeight == 16)
        tileNumber &= 0xFE;

    uint16_t tileOffset = uint16_t(tileNumber) * 16;

    // For 8x16 sprites, rows 0-7 use first tile, rows 8-15 use second tile.
    if (spriteHeight == 16 && row >= 8)
    {
        tileOffset += 16;
        row -= 8;
    }

    uint16_t rowOffset = tileOffset + row * 2;

    uint8_t lo = vram[rowOffset];
    uint8_t hi = vram[rowOffset + 1];

    for (int pixel = 0; pixel < 8; pixel++)
    {
        int targetX = screenX + pixel;

        if (targetX < 0 || targetX >= 160)
            continue;

        int bitIndex = xFlip ? pixel : 7 - pixel;

        uint8_t lowBit  = (lo >> bitIndex) & 0x01;
        uint8_t highBit = (hi >> bitIndex) & 0x01;

        uint8_t colorId = lowBit | (highBit << 1);

        // Sprite color 0 is transparent.
        if (colorId == 0)
            continue;

        // OBJ-to-BG priority.
        // If set, sprite is hidden behind BG colors 1-3.
        // It still shows over BG color 0.
        if (priorityBehindBG && bgColorIds[targetX] != 0)
            continue;

        uint8_t palette = useOBP1 ? obp1 : obp0;

        uint8_t shade = (palette >> (colorId * 2)) & 0x03;

        framebuffer[line * 160 + targetX] = dmgShadeToRGB(shade);
    }
}

uint8_t PPU::fetchWindowPixel(int x, int y)
{
    if (!isWindowEnabled() || !isBGWindowEnabled())
        return 0;

    int windowLeft = int(wx) - 7;
    int windowTop  = int(wy);

    if (x < windowLeft || y < windowTop)
        return 0;

    int windowX = x - windowLeft;
    int windowY = y - windowTop;

    int tileCol = windowX / 8;
    int tileRow = windowY / 8;

    int pixelX = windowX % 8;
    int pixelY = windowY % 8;

    uint16_t tileMapBase = (lcdc & 0x40) ? 0x1C00 : 0x1800;

    uint16_t mapIndex = tileRow * 32 + tileCol;

    uint8_t tileNumber = vram[tileMapBase + mapIndex];

    uint16_t tileDataAddress;

    if (lcdc & 0x10)
        tileDataAddress = tileNumber * 16;
    else
        tileDataAddress = uint16_t(0x1000 + int16_t(int8_t(tileNumber)) * 16);

    uint16_t rowAddress = tileDataAddress + pixelY * 2;

    uint8_t lowByte  = vram[rowAddress];
    uint8_t highByte = vram[rowAddress + 1];

    int bit = 7 - pixelX;

    uint8_t lowBit  = (lowByte  >> bit) & 1;
    uint8_t highBit = (highByte >> bit) & 1;

    return lowBit | (highBit << 1);
}

void PPU::renderSpritesOnScanline(uint8_t line, const uint8_t bgColorIds[160])
{
    SpriteEntry sprites[10];
    uint8_t spriteCount = 0;

    const uint8_t spriteHeight = getSpriteHeight();

    // Find up to 10 sprites touching this scanline.
    for (uint8_t i = 0; i < 40; i++)
    {
        uint16_t base = i * 4;

        uint8_t spriteY = oam[base + 0];
        uint8_t spriteX = oam[base + 1];
        uint8_t tile    = oam[base + 2];
        uint8_t attr    = oam[base + 3];

        int screenY = int(spriteY) - 16;

        if (line >= screenY && line < screenY + spriteHeight)
        {
            sprites[spriteCount] = { i, spriteY, spriteX, tile, attr };
            spriteCount++;

            if (spriteCount == 10)
                break;
        }
    }

    // DMG sprite priority: lower X wins, then lower OAM index.
    // Since we draw later sprites over earlier sprites, draw lowest priority first.
    for (uint8_t i = 0; i < spriteCount; i++)
    {
        for (uint8_t j = i + 1; j < spriteCount; j++)
        {
            bool jHasHigherPriority = false;

            if (sprites[j].x < sprites[i].x)
                jHasHigherPriority = true;
            else if (sprites[j].x == sprites[i].x && sprites[j].index < sprites[i].index)
                jHasHigherPriority = true;

            // We want lower priority first, higher priority later.
            if (jHasHigherPriority)
            {
                SpriteEntry temp = sprites[i];
                sprites[i] = sprites[j];
                sprites[j] = temp;
            }
        }
    }

    // Draw from lowest priority to highest priority.
    // Because the above sort puts highest priority first, iterate backwards.
    for (int s = spriteCount - 1; s >= 0; s--)
    {
        drawSpriteLine(sprites[s], line, bgColorIds);
    }
}

uint32_t PPU::dmgShadeToRGB(uint8_t shade)
{
    switch (shade & 0x03)
    {
        case 0: return 0xFFFFFFFF; // white
        case 1: return 0xFFAAAAAA; // light gray
        case 2: return 0xFF555555; // dark gray
        case 3: return 0xFF000000; // black
    }

    return 0xFFFFFFFF;
}
