// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>
#include "StateReader.h"
#include "StateWriter.h"

class Joypad
{
    public:
        Joypad();
        virtual ~Joypad();

        void reset();

        void saveState(StateWriter& wrtr) const;
        bool loadState(const StateReader::Chunk& chunk, StateReader& rdr);

        uint8_t read() const;
        void write(uint8_t value);

        void setButtonA(bool pressed);
        void setButtonB(bool pressed);
        void setSelect(bool pressed);
        void setStart(bool pressed);

        void setLeft(bool pressed);
        void setRight(bool pressed);
        void setUp(bool pressed);
        void setDown(bool pressed);

    protected:

    private:
        uint8_t selectBits;

        bool a;
        bool b;
        bool select;
        bool start;

        bool left;
        bool right;
        bool up;
        bool down;
};

#endif // JOYPAD_H
