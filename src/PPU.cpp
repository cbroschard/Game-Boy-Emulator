// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Bus.h"
#include "common/IORegisters.h"
#include "PPU.h"

PPU::PPU() :
    bus(nullptr),
    hardwareMode(HardwareMode::DMG),
    frameReady(false),
    scanLineRendered(false),
    vramBankSelect(0),
    stat(0x80),
    lcdc(0x91),
    scy(0),
    scx(0),
    ly(0),
    lyc(0),
    bgp(0xFC),
    bgpi(0x00),
    obp0(0xFF),
    obp1(0xFF),
    obpi(0x00),
    objPriorityMode(0x00),
    wy(0),
    wx(0),
    dots(0),
    windowLineCounter(0),
    previousStatSignal(false)
{
    mode = PPUMode::OAM;
}

PPU::~PPU()
{

}

void PPU::reset()
{
    oam.fill(0x00);

    for (auto& bank : vram)
        bank.fill(0x00);

    bgPaletteRAM.fill(0x00);
    objPaletteRAM.fill(0x00);

    framebuffer.fill(0);

    mode                = PPUMode::OAM;

    vramBankSelect      = 0;

    stat                = 0x80 | static_cast<uint8_t>(mode);

    lcdc                = 0x91;

    scy                 = 0;
    scx                 = 0;
    ly                  = 0;
    lyc                 = 0;

    bgp                 = 0xFC;
    bgpi                = 0x00;
    obp0                = 0xFF;
    obp1                = 0xFF;
    obpi                = 0x00;

    objPriorityMode     = 0x00;

    wy                  = 0;
    wx                  = 0;
    dots                = 0;
    windowLineCounter   = 0;

    frameReady          = false;
    scanLineRendered    = false;
    previousStatSignal  = false;

    updateLYCCompare();
}

void PPU::setHardwareMode(HardwareMode mode)
{
    hardwareMode = mode;

    if (hardwareMode == HardwareMode::DMG)
    {
        vramBankSelect  = 0;
        bgpi            = 0;
        obpi            = 0;
        objPriorityMode = 0;
    }
}

void PPU::tick(int cyclesElapsed)
{
    if (!isLCDEnabled())
    {
        dots = 0;
        ly = 0;
        windowLineCounter = 0;

        frameReady = false;
        scanLineRendered = false;

        mode = PPUMode::HBlank;
        stat = (stat & 0xFC) | static_cast<uint8_t>(mode);

        if (getVisibleLY() == lyc)
            stat |= 0x04;
        else
            stat &= static_cast<uint8_t>(~0x04);

        previousStatSignal = false;
        return;
    }

    const uint16_t oldDots = dots;

    dots = static_cast<uint16_t>(dots + cyclesElapsed);

    // During scanline 153, the visible LY value changes from 153
    // to 0 at approximately dot 4.
    if (ly == 153 && oldDots < 4 && dots >= 4)
        updateLYCCompare();

    while (dots >= 456)
    {
        dots -= 456;
        scanLineRendered = false;

        if (ly == 153)
        {
            ly = 0;
            windowLineCounter = 0;

            mode = PPUMode::OAM;
            stat = (stat & 0xFC) | static_cast<uint8_t>(mode);

            updateLYCCompare();
        }
        else
        {
            ly++;

            if (ly == 144)
            {
                mode = PPUMode::VBlank;
                stat = (stat & 0xFC) | static_cast<uint8_t>(mode);

                updateLYCCompare();

                requestVBlankInterrupt();
                frameReady = true;
            }
            else
            {
                mode = PPUMode::OAM;
                stat = (stat & 0xFC) | static_cast<uint8_t>(mode);

                updateLYCCompare();
            }
        }
    }

    if (ly >= 144)
    {
        setMode(PPUMode::VBlank);
        return;
    }

    if (dots >= 80 && !scanLineRendered)
    {
        renderScanline(ly);
        scanLineRendered = true;
    }

    if (dots < 80)
        setMode(PPUMode::OAM);
    else if (dots < 252)
        setMode(PPUMode::Drawing);
    else
        setMode(PPUMode::HBlank);
}

void PPU::saveState(StateWriter& wrtr) const
{
    wrtr.beginChunk("PPU0");

    // Version
    wrtr.writeU32(4);

    wrtr.writeArrayU8(vram[0].data(), vram[0].size());
    wrtr.writeArrayU8(vram[1].data(), vram[1].size());
    wrtr.writeArrayU8(oam.data(), oam.size());
    wrtr.writeArrayU8(bgPaletteRAM.data(), bgPaletteRAM.size());
    wrtr.writeArrayU8(objPaletteRAM.data(), objPaletteRAM.size());

    wrtr.writeArrayU32(framebuffer.data(), framebuffer.size());

    wrtr.writeU8(static_cast<uint8_t>(mode));

    wrtr.writeU8(vramBankSelect);

    wrtr.writeBool(frameReady);
    wrtr.writeBool(scanLineRendered);

    wrtr.writeU8(stat);
    wrtr.writeU8(lcdc);

    wrtr.writeU8(scy);
    wrtr.writeU8(scx);
    wrtr.writeU8(ly);
    wrtr.writeU8(lyc);

    wrtr.writeU8(bgp);
    wrtr.writeU8(bgpi);
    wrtr.writeU8(obp0);
    wrtr.writeU8(obp1);
    wrtr.writeU8(obpi);

    wrtr.writeU8(objPriorityMode);

    wrtr.writeU8(wy);
    wrtr.writeU8(wx);

    wrtr.writeU16(dots);
    wrtr.writeU8(windowLineCounter);
    wrtr.writeBool(previousStatSignal);

    wrtr.endChunk();
}

bool PPU::loadState(const StateReader::Chunk& chunk, StateReader& rdr)
{
    rdr.enterChunkPayload(chunk);

    if (std::memcmp(chunk.tag, "PPU0", 4) != 0)                         { rdr.exitChunkPayload(chunk); return false; }

    uint32_t version = 0;

    if (!rdr.readU32(version))                                          { rdr.exitChunkPayload(chunk); return false; }

    if (version != 4)                                                   { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readArrayU8(vram[0].data(), vram[0].size()))               { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readArrayU8(vram[1].data(), vram[1].size()))               { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readArrayU8(oam.data(), oam.size()))                       { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readArrayU8(bgPaletteRAM.data(), bgPaletteRAM.size()))     { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readArrayU8(objPaletteRAM.data(), objPaletteRAM.size()))   { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readArrayU32(framebuffer.data(), framebuffer.size()))      { rdr.exitChunkPayload(chunk); return false; }

    uint8_t tempMode = 0;

    if (!rdr.readU8(tempMode))                                          { rdr.exitChunkPayload(chunk); return false; }

    mode = static_cast<PPUMode>(tempMode);

    if (!rdr.readU8(vramBankSelect))                                    { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readBool(frameReady))                                      { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(scanLineRendered))                                { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(stat))                                              { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(lcdc))                                              { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(scy))                                               { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(scx))                                               { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(ly))                                                { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(lyc))                                               { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(bgp))                                               { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(bgpi))                                              { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(obp0))                                              { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(obp1))                                              { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(obpi))                                              { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(objPriorityMode))                                   { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(wy))                                                { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(wx))                                                { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU16(dots))                                             { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(windowLineCounter))                                 { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readBool(previousStatSignal))                              { rdr.exitChunkPayload(chunk); return false; }

    vramBankSelect  &= 0x01;
    bgpi            &= 0xBF;
    obpi            &= 0xBF;
    objPriorityMode &= 0x01;

    stat |= 0x80;
    stat = (stat & 0xFC) | static_cast<uint8_t>(mode);

    if (getVisibleLY() == lyc)
        stat |= 0x04;
    else
        stat &= static_cast<uint8_t>(~0x04);

    frameReady = false;
    scanLineRendered = false;

    rdr.exitChunkPayload(chunk);

    return true;
}

uint8_t PPU::readVRAM(uint16_t offset) const
{
    if (offset >= 0x2000)
        return 0xFF;

    const uint8_t bank =
        hardwareMode == HardwareMode::CGB
            ? (vramBankSelect & 0x01)
            : 0;

    return vram[bank][offset];
}

void PPU::writeVRAM(uint16_t offset, uint8_t value)
{
    if (offset >= 0x2000)
        return;

    const uint8_t bank = hardwareMode == HardwareMode::CGB ? (vramBankSelect & 0x01) : 0;

    vram[bank][offset] = value;
}

void PPU::writeVRAMFromDMA(uint16_t offset, uint8_t value)
{
    if (offset >= 0x2000)
        return;

    const uint8_t bank =
        hardwareMode == HardwareMode::CGB
            ? (vramBankSelect & 0x01)
            : 0;

    vram[bank][offset] = value;
}

uint8_t PPU::readRegister(uint16_t address) const
{
    switch (address)
    {
        case IORegisters::PPU::LCDC:
            return lcdc;

        case IORegisters::PPU::STAT:
            return stat | 0x80;

        case IORegisters::PPU::SCY:
            return scy;

        case IORegisters::PPU::SCX:
            return scx;

        case IORegisters::PPU::LY:
            return getVisibleLY();

        case IORegisters::PPU::LYC:
            return lyc;

        case IORegisters::PPU::BGP:
            return bgp;

        case IORegisters::PPU::OBP0:
            return obp0;

        case IORegisters::PPU::OBP1:
            return obp1;

        case IORegisters::PPU::WY:
            return wy;

        case IORegisters::PPU::WX:
            return wx;

        case IORegisters::PPU::VBK:
        {
            if (hardwareMode == HardwareMode::CGB)
                return static_cast<uint8_t>(0xFE | (vramBankSelect & 0x01));
            return 0xFF;
        }

        case IORegisters::PPU::BGPI:
            if (hardwareMode == HardwareMode::CGB)
                return bgpi | 0x40;
            return 0xFF;

        case IORegisters::PPU::BGPD:
            if (hardwareMode == HardwareMode::CGB && mode != PPUMode::Drawing)
                return bgPaletteRAM[bgpi & 0x3F];
            return 0xFF;

        case IORegisters::PPU::OBPI:
            if (hardwareMode == HardwareMode::CGB)
                return obpi | 0x40;
            return 0xFF;

        case IORegisters::PPU::OBPD:
            if (hardwareMode == HardwareMode::CGB && mode != PPUMode::Drawing)
                return objPaletteRAM[obpi & 0x3F];
            return 0xFF;

        case IORegisters::PPU::OPRI:
            if (hardwareMode == HardwareMode::CGB)
                return 0xFE | (objPriorityMode & 0x01);
            return 0xFF;

        default:
            return 0xFF;
    }
}

void PPU::writeRegister(uint16_t address, uint8_t value)
{
    switch (address)
    {
        case IORegisters::PPU::LCDC:
        {
            const bool wasEnabled = isLCDEnabled();

            lcdc = value;

            const bool nowEnabled = isLCDEnabled();

            if (wasEnabled && !nowEnabled)
            {
                dots = 0;
                ly = 0;
                windowLineCounter = 0;

                frameReady = false;
                scanLineRendered = false;

                mode = PPUMode::HBlank;
                stat = (stat & 0xFC) | static_cast<uint8_t>(mode);

                if (getVisibleLY() == lyc)
                    stat |= 0x04;
                else
                    stat &= static_cast<uint8_t>(~0x04);

                previousStatSignal = false;
            }
            else if (!wasEnabled && nowEnabled)
            {
                dots = 0;
                ly = 0;
                windowLineCounter = 0;

                frameReady = false;
                scanLineRendered = false;

                mode = PPUMode::OAM;
                stat = (stat & 0xFC) | static_cast<uint8_t>(mode);

                previousStatSignal = false;

                updateLYCCompare();
            }

            return;
        }

        case IORegisters::PPU::STAT:
        {
            stat = (stat & 0x87) | (value & 0x78);

            updateStatInterruptSignal();
            return;
        }

        case IORegisters::PPU::SCY:
        {
            scy = value;
            return;
        }

        case IORegisters::PPU::SCX:
        {
            scx = value;
            return;
        }

        case IORegisters::PPU::LY:
        {
            return;
        }

        case IORegisters::PPU::LYC:
        {
            lyc = value;
            updateLYCCompare();
            return;
        }

        case IORegisters::PPU::BGP:
        {
            bgp = value;
            return;
        }

        case IORegisters::PPU::OBP0:
        {
            obp0 = value;
            return;
        }

        case IORegisters::PPU::OBP1:
        {
            obp1 = value;
            return;
        }

        case IORegisters::PPU::WY:
        {
            wy = value;
            return;
        }

        case IORegisters::PPU::WX:
        {
            wx = value;
            return;
        }

        case IORegisters::PPU::VBK:
        {
            if (hardwareMode == HardwareMode::CGB)
                vramBankSelect = value & 0x01;
            return;
        }

        case IORegisters::PPU::BGPI:
        {
            if (hardwareMode == HardwareMode::CGB)
                bgpi = value & 0xBF; // Keep bit 7 and bits 0-5, clear bit 6
            return;
        }

        case IORegisters::PPU::BGPD:
        {
            if (hardwareMode == HardwareMode::CGB &&
                mode != PPUMode::Drawing)
            {
                bgPaletteRAM[bgpi & 0x3F] = value;

                if (bgpi & 0x80)
                {
                    const uint8_t nextIndex = ((bgpi & 0x3F) + 1) & 0x3F;

                    bgpi = (bgpi & 0x80) | nextIndex;
                }
            }

            return;
        }

        case IORegisters::PPU::OBPI:
        {
            if (hardwareMode == HardwareMode::CGB)
                obpi = value & 0xBF; // Keep bit 7 and bits 0-5, clear bit 6
            return;
        }

        case IORegisters::PPU::OBPD:
        {
            if (hardwareMode == HardwareMode::CGB &&
                mode != PPUMode::Drawing)
            {
                objPaletteRAM[obpi & 0x3F] = value;

                if (obpi & 0x80)
                {
                    const uint8_t nextIndex = ((obpi & 0x3F) + 1) & 0x3F;

                    obpi = (obpi & 0x80) | nextIndex;
                }
            }

            return;
        }

        case IORegisters::PPU::OPRI:
            if (hardwareMode == HardwareMode::CGB)
                objPriorityMode = value & 0x01;
            return;

        default:
            return;
    }
}

uint8_t PPU::getVisibleLY() const
{
    // Internally the PPU remains on scanline 153, but LY reads as
    // zero beginning at approximately dot 4.
    if (ly == 153 && dots >= 4)
        return 0;

    return ly;
}

void PPU::setMode(PPUMode newMode)
{
    if (mode == newMode)
        return;

    const PPUMode oldMode = mode;

    mode = newMode;

    stat =
        (stat & 0xFC) |
        static_cast<uint8_t>(newMode);

    updateStatInterruptSignal();

    if (newMode == PPUMode::HBlank &&
        oldMode != PPUMode::HBlank &&
        bus)
    {
        bus->onPPUHBlank();
    }
}

void PPU::updateStatInterruptSignal()
{
    if (!isLCDEnabled())
    {
        previousStatSignal = false;
        return;
    }

    const bool lycSource =
        (stat & 0x40) &&
        (getVisibleLY() == lyc);

    const bool oamSource =
        (stat & 0x20) &&
        (mode == PPUMode::OAM);

    const bool vblankSource =
        (stat & 0x10) &&
        (mode == PPUMode::VBlank);

    const bool hblankSource =
        (stat & 0x08) &&
        (mode == PPUMode::HBlank);

    const bool currentStatSignal =
        lycSource ||
        oamSource ||
        vblankSource ||
        hblankSource;

    if (!previousStatSignal && currentStatSignal)
        requestLCDStatInterrupt();

    previousStatSignal = currentStatSignal;
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
    if (getVisibleLY() == lyc)
        stat |= 0x04;
    else
        stat &= static_cast<uint8_t>(~0x04);

    updateStatInterruptSignal();
}

void PPU::renderScanline(uint8_t line)
{
    if (line >= 144)
        return;
    BGPixel bgPixels[160];

    const int windowLeft = int(wx) - 7;

    const bool bgWindowAvailable = hardwareMode == HardwareMode::CGB || isBGWindowEnabled();

    const bool windowCanRender = isWindowEnabled() && bgWindowAvailable && line >= wy && windowLeft < 160;

    bool windowRenderedThisLine = false;

    for (int x = 0; x < 160; x++)
    {
        BGPixel pixel =
        {
            0,      // colorId
            0,      // palette
            false   // priority
        };

        if (bgWindowAvailable)
            pixel = fetchBGPixel(x, line);

        if (windowCanRender && x >= windowLeft)
        {
            pixel = fetchWindowPixel(x);
            windowRenderedThisLine = true;
        }

        bgPixels[x] = pixel;

        if (hardwareMode == HardwareMode::CGB)
        {
            framebuffer[line * 160 + x] =
                cgbColorToRGB(
                    bgPaletteRAM,
                    pixel.palette,
                    pixel.colorId
                );
        }
        else
        {
            const uint8_t shade = static_cast<uint8_t>((bgp >> (pixel.colorId * 2)) & 0x03);

            framebuffer[line * 160 + x] = dmgShadeToRGB(shade);
        }
    }

    if (windowRenderedThisLine)
        windowLineCounter++;

    if (areSpritesEnabled())
        renderSpritesOnScanline(line, bgPixels);
}

PPU::BGPixel PPU::fetchBGPixel(int x, int y)
{
    BGPixel pixel =
    {
        0,      // colorId
        0,      // palette
        false   // priority
    };

    if (hardwareMode == HardwareMode::DMG &&
        !isBGWindowEnabled())
    {
        return pixel;
    }

    uint8_t bgX = static_cast<uint8_t>(scx + x);

    uint8_t bgY = static_cast<uint8_t>(scy + y);

    uint8_t tileX = bgX / 8;
    uint8_t tileY = bgY / 8;

    uint8_t pixelX = bgX % 8;
    uint8_t pixelY = bgY % 8;

    const uint16_t tileMapBase = useBGTileMapHigh() ? 0x1C00 : 0x1800;

    const uint16_t tileMapOffset = tileMapBase + static_cast<uint16_t>(tileY * 32) + tileX;

    const uint8_t tileNumber = vram[0][tileMapOffset];

    uint8_t attributes = 0;

    if (hardwareMode == HardwareMode::CGB)
        attributes = vram[1][tileMapOffset];

    pixel.palette = hardwareMode == HardwareMode::CGB ? (attributes & 0x07) : 0;

    pixel.priority = hardwareMode == HardwareMode::CGB && (attributes & 0x80) != 0;

    const uint8_t tileBank = hardwareMode == HardwareMode::CGB ? ((attributes >> 3) & 0x01) : 0;

    if (hardwareMode == HardwareMode::CGB)
    {
        if (attributes & 0x20)
            pixelX = 7 - pixelX;

        if (attributes & 0x40)
            pixelY = 7 - pixelY;
    }

    uint16_t tileDataOffset;

    if (useTileDataUnsigned())
    {
        tileDataOffset = static_cast<uint16_t>(tileNumber) * 16;
    }
    else
    {
        const int8_t signedTile = static_cast<int8_t>(tileNumber);

        tileDataOffset = static_cast<uint16_t>(0x1000 + static_cast<int16_t>(signedTile) * 16);
    }

    const uint16_t rowOffset = tileDataOffset + static_cast<uint16_t>(pixelY * 2);

    const uint8_t lo = vram[tileBank][rowOffset];

    const uint8_t hi = vram[tileBank][rowOffset + 1];

    const uint8_t bit = static_cast<uint8_t>(7 - pixelX);

    const uint8_t lowBit = (lo >> bit) & 0x01;

    const uint8_t highBit = (hi >> bit) & 0x01;

    pixel.colorId = lowBit | (highBit << 1);

    return pixel;
}

void PPU::drawSpriteLine(
    const SpriteEntry& sprite,
    uint8_t line,
    const BGPixel bgPixels[160])
{
    const uint8_t spriteHeight = getSpriteHeight();

    const int screenX = static_cast<int>(sprite.x) - 8;
    const int screenY = static_cast<int>(sprite.y) - 16;

    const bool priorityBehindBG =
        (sprite.attributes & 0x80) != 0;

    const bool yFlip =
        (sprite.attributes & 0x40) != 0;

    const bool xFlip =
        (sprite.attributes & 0x20) != 0;

    // DMG only: bit 4 selects OBP0 or OBP1.
    const bool useOBP1 =
        (sprite.attributes & 0x10) != 0;

    // CGB only: bit 3 selects sprite tile-data VRAM bank.
    const uint8_t tileBank =
        hardwareMode == HardwareMode::CGB
            ? static_cast<uint8_t>(
                (sprite.attributes >> 3) & 0x01)
            : 0;

    // CGB only: bits 0-2 select one of eight OBJ palettes.
    const uint8_t cgbPalette =
        static_cast<uint8_t>(
            sprite.attributes & 0x07);

    int row =
        static_cast<int>(line) - screenY;

    if (yFlip)
        row = spriteHeight - 1 - row;

    uint8_t tileNumber = sprite.tile;

    // In 8x16 mode, bit 0 of the tile number is ignored.
    if (spriteHeight == 16)
        tileNumber &= 0xFE;

    uint16_t tileOffset =
        static_cast<uint16_t>(tileNumber) * 16;

    if (spriteHeight == 16 && row >= 8)
    {
        tileOffset += 16;
        row -= 8;
    }

    const uint16_t rowOffset =
        tileOffset +
        static_cast<uint16_t>(row * 2);

    const uint8_t lo =
        vram[tileBank][rowOffset];

    const uint8_t hi =
        vram[tileBank][rowOffset + 1];

    for (int pixel = 0; pixel < 8; pixel++)
    {
        const int targetX =
            screenX + pixel;

        if (targetX < 0 || targetX >= 160)
            continue;

        const int bitIndex =
            xFlip
                ? pixel
                : 7 - pixel;

        const uint8_t lowBit =
            static_cast<uint8_t>(
                (lo >> bitIndex) & 0x01);

        const uint8_t highBit =
            static_cast<uint8_t>(
                (hi >> bitIndex) & 0x01);

        const uint8_t colorId =
            static_cast<uint8_t>(
                lowBit | (highBit << 1));

        // OBJ color zero is transparent.
        if (colorId == 0)
            continue;

        const bool bgOpaque =
            bgPixels[targetX].colorId != 0;

        if (hardwareMode == HardwareMode::CGB)
        {
            /*
             * In CGB mode LCDC bit 0 controls the master
             * BG/OBJ priority behavior.
             */
            const bool bgPriorityEnabled =
                (lcdc & 0x01) != 0;

            if (bgPriorityEnabled &&
                bgOpaque &&
                (bgPixels[targetX].priority ||
                 priorityBehindBG))
            {
                continue;
            }

            framebuffer[line * 160 + targetX] =
                cgbColorToRGB(
                    objPaletteRAM,
                    cgbPalette,
                    colorId
                );
        }
        else
        {
            if (priorityBehindBG && bgOpaque)
                continue;

            const uint8_t palette =
                useOBP1 ? obp1 : obp0;

            const uint8_t shade =
                static_cast<uint8_t>(
                    (palette >> (colorId * 2)) &
                    0x03
                );

            framebuffer[line * 160 + targetX] =
                dmgShadeToRGB(shade);
        }
    }
}

PPU::BGPixel PPU::fetchWindowPixel(int x)
{
    BGPixel pixel =
    {
        0,      // colorId
        0,      // palette
        false   // priority
    };

    if (!isWindowEnabled())
        return pixel;

    if (hardwareMode == HardwareMode::DMG &&
        !isBGWindowEnabled())
    {
        return pixel;
    }

    const int windowLeft = int(wx) - 7;

    if (x < windowLeft)
        return pixel;

    const int windowX = x - windowLeft;

    const int windowY = windowLineCounter;

    const int tileCol = windowX / 8;

    const int tileRow = windowY / 8;

    uint8_t pixelX = static_cast<uint8_t>(windowX % 8);

    uint8_t pixelY = static_cast<uint8_t>(windowY % 8);

    const uint16_t tileMapBase = useWindowTileMapHigh() ? 0x1C00 : 0x1800;

    const uint16_t mapIndex = static_cast<uint16_t>(tileRow * 32 + tileCol);

    const uint16_t tileMapOffset = tileMapBase + mapIndex;

    const uint8_t tileNumber = vram[0][tileMapOffset];

    uint8_t attributes = 0;

    if (hardwareMode == HardwareMode::CGB)
        attributes = vram[1][tileMapOffset];

    pixel.palette = hardwareMode == HardwareMode::CGB ? (attributes & 0x07) : 0;

    pixel.priority = hardwareMode == HardwareMode::CGB && (attributes & 0x80) != 0;

    const uint8_t tileBank = hardwareMode == HardwareMode::CGB ? ((attributes >> 3) & 0x01) : 0;

    if (hardwareMode == HardwareMode::CGB)
    {
        if (attributes & 0x20)
            pixelX = 7 - pixelX;

        if (attributes & 0x40)
            pixelY = 7 - pixelY;
    }

    uint16_t tileDataAddress;

    if (useTileDataUnsigned())
    {
        tileDataAddress = static_cast<uint16_t>(tileNumber) * 16;
    }
    else
    {
        const int8_t signedTile = static_cast<int8_t>(tileNumber);

        tileDataAddress = static_cast<uint16_t>(0x1000 + static_cast<int16_t>(signedTile) * 16);
    }

    const uint16_t rowAddress = tileDataAddress + static_cast<uint16_t>(pixelY * 2);

    const uint8_t lowByte = vram[tileBank][rowAddress];

    const uint8_t highByte = vram[tileBank][rowAddress + 1];

    const uint8_t bit = static_cast<uint8_t>(7 - pixelX);

    const uint8_t lowBit = (lowByte >> bit) & 0x01;

    const uint8_t highBit = (highByte >> bit) & 0x01;

    pixel.colorId = lowBit | (highBit << 1);

    return pixel;
}

void PPU::renderSpritesOnScanline(uint8_t line, const BGPixel bgPixels[160])
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

        if (line >= screenY &&
            line < screenY + spriteHeight)
        {
            sprites[spriteCount] =
            {
                i,
                spriteY,
                spriteX,
                tile,
                attr
            };

            spriteCount++;

            if (spriteCount == 10)
                break;
        }
    }

    // DMG sprite priority uses lower X first, followed by
    // lower OAM index when X coordinates are equal.
    for (uint8_t i = 0; i < spriteCount; i++)
    {
        for (uint8_t j = i + 1; j < spriteCount; j++)
        {
            bool jHasHigherPriority = false;

            if (sprites[j].x < sprites[i].x)
            {
                jHasHigherPriority = true;
            }
            else if (
                sprites[j].x == sprites[i].x &&
                sprites[j].index < sprites[i].index)
            {
                jHasHigherPriority = true;
            }

            if (jHasHigherPriority)
            {
                SpriteEntry temp = sprites[i];

                sprites[i] = sprites[j];
                sprites[j] = temp;
            }
        }
    }

    // Draw the lowest-priority sprite first and the
    // highest-priority sprite last.
    for (int s = spriteCount - 1; s >= 0; s--)
    {
        drawSpriteLine(
            sprites[s],
            line,
            bgPixels
        );
    }
}

uint32_t PPU::dmgShadeToRGB(uint8_t shade)
{
    switch (shade & 0x03)
    {
        case 0:
            return 0xFFFFFFFF; // White

        case 1:
            return 0xFFAAAAAA; // Light gray

        case 2:
            return 0xFF555555; // Dark gray

        case 3:
            return 0xFF000000; // Black
    }

    return 0xFFFFFFFF;
}

uint32_t PPU::cgbColorToRGB(const std::array<uint8_t, 64>& paletteRAM, uint8_t palette, uint8_t colorId)
{
    palette &= 0x07;
    colorId &= 0x03;

    const uint8_t offset = static_cast<uint8_t>(palette * 8 + colorId * 2);

    const uint16_t color = static_cast<uint16_t>(
        paletteRAM[offset] |
        (static_cast<uint16_t>(paletteRAM[offset + 1]) << 8)
    );

    const uint8_t r = color & 0x1F;
    const uint8_t g = (color >> 5) & 0x1F;
    const uint8_t b = (color >> 10) & 0x1F;

    // Apply color correction matrix to simulate the CGB TFT LCD
    const uint32_t correctedRed = (r * 26 + g * 4 + b * 2) >> 5;
    const uint32_t correctedGreen = (g * 24 + b * 8) >> 5;
    const uint32_t correctedBlue = (r * 6 + g * 4 + b * 22) >> 5;

    // Scale the corrected 5-bit values back up to 8-bit limits
    const uint8_t finalRed = static_cast<uint8_t>((correctedRed << 3) | (correctedRed >> 2));
    const uint8_t finalGreen = static_cast<uint8_t>((correctedGreen << 3) | (correctedGreen >> 2));
    const uint8_t finalBlue = static_cast<uint8_t>((correctedBlue << 3) | (correctedBlue >> 2));

    return 0xFF000000 |
           (static_cast<uint32_t>(finalRed) << 16) |
           (static_cast<uint32_t>(finalGreen) << 8) |
           static_cast<uint32_t>(finalBlue);
}
