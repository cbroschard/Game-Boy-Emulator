// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Joypad.h"

Joypad::Joypad() :
    selectBits(0x30),
    a(false),
    b(false),
    select(false),
    start(false),
    left(false),
    right(false),
    up(false),
    down(false)
{

}

Joypad::~Joypad() = default;

void Joypad::reset()
{
    // Reset to default power on state
    selectBits  = 0x30;

    a           = false;
    b           = false;
    select      = false;
    start       = false;

    left        = false;
    right       = false;
    up          = false;
    down        = false;
}

void Joypad::saveState(StateWriter& wrtr) const
{
    wrtr.beginChunk("JOY0");

    // Version
    wrtr.writeU32(1);

    wrtr.writeU8(selectBits);

    wrtr.writeBool(a);
    wrtr.writeBool(b);
    wrtr.writeBool(select);
    wrtr.writeBool(start);

    wrtr.writeBool(left);
    wrtr.writeBool(right);
    wrtr.writeBool(up);
    wrtr.writeBool(down);

    wrtr.endChunk();
}

bool Joypad::loadState(const StateReader::Chunk& chunk, StateReader& rdr)
{
    rdr.enterChunkPayload(chunk);

    if (std::memcmp(chunk.tag, "JOY0", 4) != 0)    { rdr.exitChunkPayload(chunk); return false; }

    uint32_t version = 0;
    if (!rdr.readU32(version))      { rdr.exitChunkPayload(chunk); return false; }
    if (version != 1)               { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU8(selectBits))    { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readBool(a))           { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(b))           { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(select))      { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(start))       { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readBool(left))        { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(right))       { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(up))          { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(down))        { rdr.exitChunkPayload(chunk); return false; }

    rdr.exitChunkPayload(chunk);

    return true;
}

uint8_t Joypad::read() const
{
    uint8_t result = 0xCF | selectBits;

    // Bit 5 low means button keys selected
    if ((selectBits & 0x20) == 0)
    {
        if (a)      result &= ~0x01;
        if (b)      result &= ~0x02;
        if (select) result &= ~0x04;
        if (start)  result &= ~0x08;
    }

    // Bit 4 low means direction keys selected
    if ((selectBits & 0x10) == 0)
    {
        if (right) result &= ~0x01;
        if (left)  result &= ~0x02;
        if (up)    result &= ~0x04;
        if (down)  result &= ~0x08;
    }

    return result;
}

void Joypad::write(uint8_t value)
{
    selectBits = value & 0x30;
}

void Joypad::setButtonA(bool pressed)
{
    a = pressed;
}

void Joypad::setButtonB(bool pressed)
{
    b = pressed;
}

void Joypad::setSelect(bool pressed)
{
    select = pressed;
}

void Joypad::setStart(bool pressed)
{
    start = pressed;
}

void Joypad::setLeft(bool pressed)
{
    left = pressed;
}

void Joypad::setRight(bool pressed)
{
    right = pressed;
}

void Joypad::setUp(bool pressed)
{
    up = pressed;
}

void Joypad::setDown(bool pressed)
{
    down = pressed;
}
