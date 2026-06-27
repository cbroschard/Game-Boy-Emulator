// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

class Bus;

class Timer
{
    public:
        Timer();
        virtual ~Timer();

        inline void attachBusInstance(Bus* bus) { this->bus = bus; }

        inline bool hasBus() const { return bus ? 1 : 0; }

        void reset();

        void tick(int cyclesElapsed);

        uint8_t readRegister(uint16_t address) const;
        void writeRegister(uint16_t address, uint8_t value);

    protected:

    private:
        Bus* bus;

        uint8_t div;
        uint8_t timA;
        uint8_t tma;
        uint8_t tac;

        uint16_t divCounter;
        uint16_t timACounter;

        // Helpers
        int getTimerPeriod() const;
};

#endif // TIMER_H
