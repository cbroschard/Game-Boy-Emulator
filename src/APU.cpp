// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "APU.h"
#include "AudioOutput.h"

APU::APU() :
    audioOutput(nullptr),
    apuEnabled(true),
    frameSequencerCounter(0),
    frameSequencerStep(0),
    sampleCounter(0)
{
    reset();
}

APU::~APU() = default;

void APU::reset()
{
    clearState();

    apuEnabled              = true;
    registers.nr52          = 0x80;

    frameSequencerCounter   = 0;
    frameSequencerStep      = 0;
    sampleCounter           = 0;
}

void APU::tick(int cyclesElapsed)
{
    if (!apuEnabled)
        return;

    while (cyclesElapsed > 0)
    {
        frameSequencerCounter++;

        if (frameSequencerCounter >= 8192)
        {
            frameSequencerCounter = 0;
            clockFrameSequencer();
        }

        sampleCounter++;

        if (sampleCounter >= cyclesPerSample)
        {
            sampleCounter -= cyclesPerSample;
            mixSample();
        }

        if (channel1.enabled)
        {
            if (channel1.periodDivider > 0)
                channel1.periodDivider--;

            if (channel1.periodDivider == 0)
            {
                uint16_t frequency = registers.nr13 | ((registers.nr14 & 0x07) << 8);
                channel1.periodDivider = (2048 - frequency) * 4;

                channel1.dutyPosition = (channel1.dutyPosition + 1) & 0x07;
            }
        }

        if (channel2.enabled)
        {
            if (channel2.periodDivider > 0)
                channel2.periodDivider--;

            if (channel2.periodDivider == 0)
            {
                uint16_t frequency = registers.nr23 | ((registers.nr24 & 0x07) << 8);
                channel2.periodDivider = (2048 - frequency) * 4;

                channel2.dutyPosition = (channel2.dutyPosition + 1) & 0x07;
            }
        }

        if (channel3.enabled)
        {
            if (channel3.periodDivider > 0)
                channel3.periodDivider--;

            if (channel3.periodDivider == 0)
            {
                uint16_t frequency = registers.nr33 | ((registers.nr34 & 0x07) << 8);
                channel3.periodDivider = (2048 - frequency) * 2;

                channel3.wavePosition = (channel3.wavePosition + 1) & 0x1F;
            }
        }

        if (channel4.enabled)
        {
            if (channel4.periodDivider > 0)
                channel4.periodDivider--;

            if (channel4.periodDivider == 0)
            {
                uint8_t baseDivisor = getNoiseBaseDivisor(registers.nr43 & 0x07);
                uint8_t clockShift = registers.nr43 >> 4;
                channel4.periodDivider = baseDivisor << clockShift;

                uint8_t bit0 = channel4.lfsr & 0x01;
                uint8_t bit1 = (channel4.lfsr >> 1) & 0x01;
                uint8_t xorResult = bit0 ^ bit1;

                channel4.lfsr >>= 1;
                channel4.lfsr |= (xorResult << 14);

                if (channel4.widthMode)
                {
                    channel4.lfsr &= ~(1 << 6);
                    channel4.lfsr |= (xorResult << 6);
                }
            }
        }

        cyclesElapsed--;
    }
}

uint8_t APU::readRegister(uint16_t address) const
{
    switch (address)
    {
        case 0xFF10: return registers.nr10;
        case 0xFF11: return registers.nr11;
        case 0xFF12: return registers.nr12;
        case 0xFF13: return registers.nr13;
        case 0xFF14: return registers.nr14;

        case 0xFF15: return 0xFF;

        case 0xFF16: return registers.nr21;
        case 0xFF17: return registers.nr22;
        case 0xFF18: return registers.nr23;
        case 0xFF19: return registers.nr24;

        case 0xFF1A: return registers.nr30;
        case 0xFF1B: return registers.nr31;
        case 0xFF1C: return registers.nr32;
        case 0xFF1D: return registers.nr33;
        case 0xFF1E: return registers.nr34;

        case 0xFF1F: return 0xFF;

        case 0xFF20: return registers.nr41;
        case 0xFF21: return registers.nr42;
        case 0xFF22: return registers.nr43;
        case 0xFF23: return registers.nr44;

        case 0xFF24: return registers.nr50;
        case 0xFF25: return registers.nr51;
        case 0xFF26:
        {
            uint8_t value = 0;

            if (apuEnabled)
                value |= 0x80;

            if (channel1.enabled)
                value |= 0x01;

            if (channel2.enabled)
                value |= 0x02;

            if (channel3.enabled)
                value |= 0x04;

            if (channel4.enabled)
                value |= 0x08;

            return value;
        }

        default:
            if (address >= 0xFF30 && address <= 0xFF3F)
                return registers.waveRAM[address - 0xFF30];

            return 0xFF;
    }
}

void APU::writeRegister(uint16_t address, uint8_t value)
{
    if (!apuEnabled && address != 0xFF26)
        return;

    switch (address)
    {
        case 0xFF10:
        {
            registers.nr10 = value;
            return;
        }

        case 0xFF11:
        {
            registers.nr11 = value;

            channel1.lengthCounter = 64 - (value & 0x3F);
            channel1.duty = (value >> 6) & 0x03;

            return;
        }

        case 0xFF12:
        {
            registers.nr12 = value;
            if (value & 0xF8)
            {
                channel1.dacEnabled = true;
                return;
            }

            channel1.dacEnabled = false;
            channel1.enabled    = false;
            return;
        }

        case 0xFF13:
        {
            registers.nr13 = value;
            return;
        }

        case 0xFF14:
        {
            registers.nr14 = value;

            channel1.lengthEnabled = (value & 0x40) ? true : false;

            if (value & 0x80)
            {
                channel1.enabled = channel1.dacEnabled;

                if (channel1.lengthCounter == 0)
                    channel1.lengthCounter = 64;

                uint16_t frequency = registers.nr13 | ((registers.nr14 & 0x07) << 8);
                channel1.periodDivider = (2048 - frequency) * 4;

                uint8_t period = (registers.nr12 & 0x07);
                channel1.envelopeTimer = period == 0 ? 8 : period;

                channel1.currentVolume = (registers.nr12 >> 4) & 0x0F;
            }

            return;
        }

        case 0xFF16:
        {
            registers.nr21 = value;

            channel2.lengthCounter = 64 - (value & 0x3F);
            channel2.duty = (value >> 6) & 0x03;

            return;
        }

        case 0xFF17:
        {
            registers.nr22 = value;
            if (value & 0xF8)
            {
                channel2.dacEnabled = true;
                return;
            }

            channel2.dacEnabled = false;
            channel2.enabled    = false;
            return;
        }

        case 0xFF18:
        {
            registers.nr23 = value;
            return;
        }

        case 0xFF19:
        {
            registers.nr24 = value;

            channel2.lengthEnabled = (value & 0x40) ? true : false;

            if (value & 0x80)
            {
                if (channel2.dacEnabled)
                    channel2.enabled = true;
                else
                    channel2.enabled = false;

                if (channel2.lengthCounter == 0)
                    channel2.lengthCounter = 64;

                uint16_t frequency = registers.nr23 | ((registers.nr24 & 0x07) << 8);
                channel2.periodDivider = (2048 - frequency) * 4;

                uint8_t period = (registers.nr22 & 0x07);
                channel2.envelopeTimer = period == 0 ? 8 : period;

                channel2.currentVolume = (registers.nr22 >> 4) & 0x0F;
            }

            return;
        }

        case 0xFF1A:
        {
            registers.nr30 = value;
            if (value & 0x80)
            {
                channel3.dacEnabled = true;
                return;
            }

            channel3.dacEnabled = false;
            channel3.enabled    = false;
            return;
        }

        case 0xFF1B:
        {
            registers.nr31 = value;

            channel3.lengthCounter = 256 - value;

            return;
        }

        case 0xFF1C:
        {
            registers.nr32 = value;

            channel3.outputLevel = (value >> 5) & 0x03;

            return;
        }

        case 0xFF1D:
        {
            registers.nr33 = value;
            return;
        }

        case 0xFF1E:
        {
            registers.nr34 = value;
            channel3.lengthEnabled = (value & 0x40) ? true : false;

            if (value & 0x80)
            {
                channel3.enabled = channel3.dacEnabled;

                if (channel3.lengthCounter == 0)
                    channel3.lengthCounter = 256;

                uint16_t frequency = registers.nr33 | ((registers.nr34 & 0x07) << 8);
                channel3.periodDivider = (2048 - frequency) * 2;

                channel3.outputLevel = (registers.nr32 >> 5) & 0x03;

                channel3.wavePosition = 0;
            }

            return;
        }

        case 0xFF20:
        {
            registers.nr41 = value;

            channel4.lengthCounter = 64 - (value & 0x3F);

            return;
        }

        case 0xFF21:
        {
            registers.nr42 = value;
            if (value & 0xF8)
            {
                channel4.dacEnabled = true;
                return;
            }

            channel4.dacEnabled = false;
            channel4.enabled    = false;
            return;
        }

        case 0xFF22:
        {
            registers.nr43 = value;

            channel4.widthMode = (value >> 3) & 0x01;

            return;
        }

        case 0xFF23:
        {
            registers.nr44 = value;

            channel4.lengthEnabled = (value & 0x40) ? true : false;

            if (value & 0x80)
            {
                channel4.enabled = channel4.dacEnabled;

                if (channel4.lengthCounter == 0)
                    channel4.lengthCounter = 64;

                uint8_t period = (registers.nr42 & 0x07);
                channel4.envelopeTimer = period == 0 ? 8 : period;

                channel4.currentVolume = (registers.nr42 >> 4) & 0x0F;

                channel4.lfsr = 0x7FFF;

                uint8_t baseDivisor = getNoiseBaseDivisor(registers.nr43 & 0x07);
                uint8_t clockShift = (registers.nr43 >> 4);
                channel4.periodDivider = (baseDivisor << clockShift);
            }

            return;
        }

        case 0xFF24:
        {
            registers.nr50 = value;
            return;
        }

        case 0xFF25:
        {
            registers.nr51 = value;
            return;
        }

        case 0xFF26:
        {
            registers.nr52 = value & 0x80;
            (value & 0x80) ? powerOn() : powerOff();
            return;
        }

        default:
        {
            if (address >= 0xFF30 && address <= 0xFF3F)
            {
                registers.waveRAM[address - 0xFF30] = value;
                return;
            }
        }
    }
}

void APU::powerOn()
{
   apuEnabled       = true;
}

void APU::powerOff()
{
    clearState();

    apuEnabled = false;
    registers.nr52 = 0x00;
}

void APU::clockFrameSequencer()
{
    switch (frameSequencerStep)
    {
        case 0:
            clockLengthCounters();
            break;

        case 1:
            break;

        case 2:
        {
            clockLengthCounters();
            clockSweep();
            break;
        }

        case 3:
            break;

        case 4:
            clockLengthCounters();
            break;

        case 5:
            break;

        case 6:
        {
            clockLengthCounters();
            clockSweep();
            break;
        }

        case 7:
            clockEnvelopes();
            break;
    }

    frameSequencerStep = (frameSequencerStep + 1) & 0x07;
}

void APU::clockLengthCounters()
{
    if (channel1.lengthEnabled && channel1.lengthCounter > 0)
    {
        channel1.lengthCounter--;

        if (channel1.lengthCounter == 0)
            channel1.enabled = false;

    }

    if (channel2.lengthEnabled && channel2.lengthCounter > 0)
    {
        channel2.lengthCounter--;

        if (channel2.lengthCounter == 0)
            channel2.enabled = false;
    }

    if (channel3.lengthEnabled && channel3.lengthCounter > 0)
    {
        channel3.lengthCounter--;

        if (channel3.lengthCounter == 0)
            channel3.enabled = false;
    }

    if (channel4.lengthEnabled && channel4.lengthCounter > 0)
    {
        channel4.lengthCounter--;

        if (channel4.lengthCounter == 0)
            channel4.enabled = false;
    }
}

void APU::clockEnvelopes()
{
    uint8_t period = registers.nr12 & 0x07;

    if (period != 0 && channel1.envelopeTimer > 0)
    {
        channel1.envelopeTimer--;

        if (channel1.envelopeTimer == 0)
        {
            channel1.envelopeTimer = period;

            bool increase = (registers.nr12 & 0x08) != 0;

            if (increase)
            {
                if (channel1.currentVolume < 15)
                    channel1.currentVolume++;
            }
            else
            {
                if (channel1.currentVolume > 0)
                    channel1.currentVolume--;
            }
        }
    }

    period = registers.nr22 & 0x07;

    if (period != 0 && channel2.envelopeTimer > 0)
    {
        channel2.envelopeTimer--;

        if (channel2.envelopeTimer == 0)
        {
            channel2.envelopeTimer = period;

            bool increase = (registers.nr22 & 0x08) != 0;

            if (increase)
            {
                if (channel2.currentVolume < 15)
                    channel2.currentVolume++;
            }
            else
            {
                if (channel2.currentVolume > 0)
                    channel2.currentVolume--;
            }
        }
    }

    period = registers.nr42 & 0x07;

    if (period != 0 && channel4.envelopeTimer > 0)
    {
        channel4.envelopeTimer--;

        if (channel4.envelopeTimer == 0)
        {
            channel4.envelopeTimer = period;

            bool increase = (registers.nr42 & 0x08) != 0;

            if (increase)
            {
                if (channel4.currentVolume < 15)
                    channel4.currentVolume++;
            }
            else
            {
                if (channel4.currentVolume > 0)
                    channel4.currentVolume--;
            }
        }
    }
}

void APU::clockSweep()
{

}

int APU::getChannel1Output() const
{
    if (!channel1.enabled || !channel1.dacEnabled)
        return 0;

    uint8_t pattern = getDutyPattern(channel1.duty);
    bool high = (pattern >> channel1.dutyPosition) & 0x01;

    return high ? channel1.currentVolume : 0;
}

int APU::getChannel2Output() const
{
    if (!channel2.enabled || !channel2.dacEnabled)
        return 0;

    uint8_t pattern = getDutyPattern(channel2.duty);
    bool high = (pattern >> channel2.dutyPosition) & 0x01;

    return high ? channel2.currentVolume : 0;
}

int APU::getChannel3Output() const
{
    if (!channel3.enabled || !channel3.dacEnabled)
        return 0;

    uint8_t byteIndex = channel3.wavePosition / 2;
    uint8_t waveByte = registers.waveRAM[byteIndex];

    uint8_t sample = 0;

    if ((channel3.wavePosition & 0x01) == 0)
        sample = waveByte >> 4;
    else
        sample = waveByte & 0x0F;

    switch (channel3.outputLevel)
    {
        case 0: return 0;
        case 1: return sample;
        case 2: return sample >> 1;
        case 3: return sample >> 2;
        default: return 0;
    }
}

int APU::getChannel4Output() const
{
    if (!channel4.enabled || !channel4.dacEnabled)
        return 0;

    if ((channel4.lfsr & 0x01) == 0)
        return channel4.currentVolume;
    else
        return 0;
}

void APU::mixSample()
{
    if (!audioOutput)
        return;

    int ch1 = getChannel1Output();
    int ch2 = getChannel2Output();
    int ch3 = getChannel3Output();
    int ch4 = getChannel4Output();

    int right = 0;
    int left  = 0;

    // NR51 bits 0-3 route channels 1-4 to right
    if (registers.nr51 & 0x01) right += ch1;
    if (registers.nr51 & 0x02) right += ch2;
    if (registers.nr51 & 0x04) right += ch3;
    if (registers.nr51 & 0x08) right += ch4;

    // NR51 bits 4-7 route channels 1-4 to left
    if (registers.nr51 & 0x10) left += ch1;
    if (registers.nr51 & 0x20) left += ch2;
    if (registers.nr51 & 0x40) left += ch3;
    if (registers.nr51 & 0x80) left += ch4;

    int rightVolume = (registers.nr50 & 0x07) + 1;
    int leftVolume  = ((registers.nr50 >> 4) & 0x07) + 1;

    right *= rightVolume;
    left  *= leftVolume;

    constexpr int SCALE = 64;

    int16_t leftSample  = static_cast<int16_t>(left * SCALE);
    int16_t rightSample = static_cast<int16_t>(right * SCALE);

    audioOutput->queueSample(leftSample, rightSample);
}

void APU::clearState()
{
    // Clear registers
    registers.nr10 = 0x00;
    registers.nr11 = 0x00;
    registers.nr12 = 0x00;
    registers.nr13 = 0x00;
    registers.nr14 = 0x00;

    registers.nr21 = 0x00;
    registers.nr22 = 0x00;
    registers.nr23 = 0x00;
    registers.nr24 = 0x00;

    registers.nr30 = 0x00;
    registers.nr31 = 0x00;
    registers.nr32 = 0x00;
    registers.nr33 = 0x00;
    registers.nr34 = 0x00;

    registers.nr41 = 0x00;
    registers.nr42 = 0x00;
    registers.nr43 = 0x00;
    registers.nr44 = 0x00;

    registers.nr50 = 0x00;
    registers.nr51 = 0x00;
    registers.nr52 = 0x00;

    for (int i = 0; i < 16; i++)
        registers.waveRAM[i] = 0x00;

    // Clear channel 1
    channel1.enabled       = false;
    channel1.dacEnabled    = false;
    channel1.lengthEnabled = false;
    channel1.lengthCounter = 0;
    channel1.periodDivider = 0;
    channel1.duty          = 0;
    channel1.dutyPosition  = 0;
    channel1.currentVolume = 0;
    channel1.envelopeTimer = 0;

    // Clear channel 2
    channel2.enabled       = false;
    channel2.dacEnabled    = false;
    channel2.lengthEnabled = false;
    channel2.lengthCounter = 0;
    channel2.periodDivider = 0;
    channel2.duty          = 0;
    channel2.dutyPosition  = 0;
    channel2.currentVolume = 0;
    channel2.envelopeTimer = 0;

    // Clear channel 3
    channel3.enabled       = false;
    channel3.dacEnabled    = false;
    channel3.lengthEnabled = false;
    channel3.lengthCounter = 0;
    channel3.periodDivider = 0;
    channel3.wavePosition  = 0;
    channel3.outputLevel   = 0;

    // Clear channel 4
    channel4.enabled       = false;
    channel4.dacEnabled    = false;
    channel4.lengthEnabled = false;
    channel4.lengthCounter = 0;
    channel4.periodDivider = 0;
    channel4.currentVolume = 0;
    channel4.lfsr          = 0;
    channel4.envelopeTimer = 0;
    channel4.widthMode     = false;
}

uint8_t APU::getNoiseBaseDivisor(uint8_t value) const
{
    switch (value)
    {
        case 0: return 8;
        case 1: return 16;
        case 2: return 32;
        case 3: return 48;
        case 4: return 64;
        case 5: return 80;
        case 6: return 96;
        case 7: return 112;
        default: return 8;
    }
}

uint8_t APU::getDutyPattern(uint8_t value) const
{
    switch (value)
    {
        case 0: return 0b00000001;
        case 1: return 0b10000001;
        case 2: return 0b10000111;
        case 3: return 0b01111110;
        default: return 0b00000001;
    }
}
