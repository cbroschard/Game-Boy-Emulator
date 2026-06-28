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

        inline bool hasAudioOutput() const { return audioOutput ? 1 : 0; }

        void reset();

        void tick(int cyclesElapsed);

        uint8_t readRegister(uint16_t address) const;
        void writeRegister(uint16_t address, uint8_t value);

    protected:

    private:
        AudioOutput* audioOutput;

        struct Registers
        {
            uint8_t nr10;
            uint8_t nr11;
            uint8_t nr12;
            uint8_t nr13;
            uint8_t nr14;

            uint8_t nr21;
            uint8_t nr22;
            uint8_t nr23;
            uint8_t nr24;

            uint8_t nr30;
            uint8_t nr31;
            uint8_t nr32;
            uint8_t nr33;
            uint8_t nr34;

            uint8_t nr41;
            uint8_t nr42;
            uint8_t nr43;
            uint8_t nr44;

            uint8_t nr50;
            uint8_t nr51;
            uint8_t nr52;

            uint8_t waveRAM[16];
        } registers;

        bool apuEnabled;

        struct APUChannel
        {
            bool enabled;
            bool dacEnabled;

            bool lengthEnabled;
            uint16_t lengthCounter;

            uint16_t periodDivider;
        };

        struct SquareChannel : APUChannel
        {
            uint8_t duty;
            uint8_t dutyPosition;
            uint8_t currentVolume;
            uint8_t envelopeTimer;
        };

        struct WaveChannel : APUChannel
        {
            uint8_t wavePosition;
            uint8_t outputLevel;
        };

        struct NoiseChannel : APUChannel
        {
            uint8_t currentVolume;
            uint16_t lfsr;
            uint8_t envelopeTimer;
            bool widthMode;
        };

        SquareChannel channel1;
        SquareChannel channel2;
        WaveChannel channel3;
        NoiseChannel channel4;

        int frameSequencerCounter;
        uint8_t frameSequencerStep;
        int sampleCounter;

        static constexpr int cyclesPerSample = 95;

        void powerOn();
        void powerOff();

        // Tick helpers
        void clockFrameSequencer();
        void clockLengthCounters();
        void clockEnvelopes();
        void clockSweep();

        int getChannel1Output() const;
        int getChannel2Output() const;
        int getChannel3Output() const;
        int getChannel4Output() const;

        // Sample generation
        void mixSample();

        // Helpers
        void clearState();
        uint8_t getNoiseBaseDivisor(uint8_t value) const;
        uint8_t getDutyPattern(uint8_t value) const;
};

#endif // APU_H
