// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Bus.h"
#include "Timer.h"

Timer::Timer() :
    bus(nullptr),
    div(0x00),
    timA(0x00),
    tma(0x00),
    tac(0x00),
    divCounter(0),
    timACounter(0)
{

}

Timer::~Timer()
{

}

void Timer::reset()
{
    div         = 0x00;
    timA        = 0x00;
    tma         = 0x00;
    tac         = 0x00;

    divCounter  = 0;
    timACounter = 0;
}

void Timer::tick(int cycles)
{
    divCounter += cycles;

    while (divCounter >= 256)
    {
        divCounter -= 256;
        div++;
    }

    if ((tac & 0x04) == 0)
        return;

    timACounter += cycles;

    int period = getTimerPeriod();

    while (timACounter >= period)
    {
        timACounter -= period;

        if (timA == 0xFF)
        {
            timA = tma;

            if (bus)
                bus->requestInterrupt(Bus::Interrupt::Timer);
        }
        else
        {
            timA++;
        }
    }
}

uint8_t Timer::readRegister(uint16_t address) const
{
    switch (address)
    {
        case 0xFF04:
            return div;
        case 0xFF05:
            return timA;
        case 0xFF06:
            return tma;
        case 0xFF07:
            return tac | 0xF8;
        default:
            return 0xFF;
    }
}

void Timer::writeRegister(uint16_t address, uint8_t value)
{
    switch (address)
    {
        case 0xFF04:
        {
            div = value;
            return;
        }

        case 0xFF05:
        {
            timA = value;
            return;
        }

        case 0xFF06:
        {
            tma = value;
            return;
        }

        case 0xFF07:
        {
            tac = value & 0x07;
            return;
        }
    }
}

int Timer::getTimerPeriod() const
{
    switch (tac & 0x03)
    {
        case 0x00: return 1024; // 4096 Hz
        case 0x01: return 16;   // 262144 Hz
        case 0x02: return 64;   // 65536 Hz
        case 0x03: return 256;  // 16384 Hz
    }

    return 1024;
}
