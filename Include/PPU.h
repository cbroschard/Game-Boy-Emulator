// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef PPU_H
#define PPU_H

#include <cstdint>

class VideoOutput;

class PPU
{
    public:
        PPU();
        virtual ~PPU();

        inline void attachVideoOutputInstance(VideoOutput* videoOutput) { this->videoOutput = videoOutput; }
        uint8_t readRegister(uint16_t address);
        void writeRegister(uint16_t address, uint8_t value);

    protected:

    private:
        VideoOutput* videoOutput;
};

#endif // PPU_H
