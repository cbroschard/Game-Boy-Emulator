// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef APU_H
#define APU_H

#include <cstdint>

class AudioOutput;

class APU
{
    public:
        APU();
        virtual ~APU();

        inline void attachAudioOutputInstance(AudioOutput* audioOutput) { this->audioOutput = audioOutput; }

        void tick(int cycles);

        uint8_t readRegister(uint16_t address);
        void writeRegister(uint16_t address, uint8_t value);

    protected:

    private:
        AudioOutput* audioOutput;
};

#endif // APU_H
