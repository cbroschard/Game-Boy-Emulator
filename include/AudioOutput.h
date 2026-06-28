// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <cstdint>
#include <vector>
#include "SDL3/SDL.h"

class AudioOutput
{
    public:
        AudioOutput();
        virtual ~AudioOutput();

        bool playAudio();
        void pauseAudio();
        void resumeAudio();
        void stopAudio();

        void queueSample(int16_t left, int16_t right);

        inline int getSampleRate() const { return SAMPLE_RATE; }
        inline int getBufferSize() const { return BUFFER_SIZE; }

    private:
        static const int SAMPLE_RATE = 44100;
        static const int CHANNELS = 2;
        static const int BUFFER_SIZE = 2048;

        SDL_AudioSpec desired{};
        SDL_AudioStream* stream;

        std::vector<int16_t> pendingSamples;

        bool initialized;

        void flushPendingSamples();
};

#endif // AUDIOOUTPUT_H
