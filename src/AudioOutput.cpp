// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.

#include <stdexcept>
#include "AudioOutput.h"

AudioOutput::AudioOutput() :
    stream(nullptr),
    initialized(false)
{
    desired.freq = SAMPLE_RATE;
    desired.format = SDL_AUDIO_S16;
    desired.channels = CHANNELS;

    pendingSamples.reserve(BUFFER_SIZE * CHANNELS);

    if (!SDL_Init(SDL_INIT_AUDIO))
    {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
}

AudioOutput::~AudioOutput()
{
    stopAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool AudioOutput::playAudio()
{
    if (initialized)
    {
        resumeAudio();
        return true;
    }

    desired.freq = SAMPLE_RATE;
    desired.format = SDL_AUDIO_S16;
    desired.channels = CHANNELS;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &desired,
                                       nullptr,
                                       nullptr);

    if (!stream)
    {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
        return false;
    }

    initialized = true;

    if (!SDL_ResumeAudioStreamDevice(stream))
    {
        SDL_Log("SDL_ResumeAudioStreamDevice failed: %s", SDL_GetError());
        stopAudio();
        return false;
    }

    return true;
}

void AudioOutput::pauseAudio()
{
    if (stream)
        SDL_PauseAudioStreamDevice(stream);
}

void AudioOutput::resumeAudio()
{
    if (stream)
        SDL_ResumeAudioStreamDevice(stream);
}

void AudioOutput::stopAudio()
{
    if (stream)
    {
        flushPendingSamples();

        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }

    pendingSamples.clear();
    initialized = false;
}

void AudioOutput::queueSample(int16_t left, int16_t right)
{
    if (!stream)
        return;

    pendingSamples.push_back(left);
    pendingSamples.push_back(right);

    if (pendingSamples.size() >= BUFFER_SIZE * CHANNELS)
        flushPendingSamples();
}

void AudioOutput::flushPendingSamples()
{
    if (!stream || pendingSamples.empty())
        return;

    const int queuedBytes = SDL_GetAudioStreamQueued(stream);
    const int maxQueuedBytes = BUFFER_SIZE * CHANNELS * static_cast<int>(sizeof(int16_t)) * 4;

    if (queuedBytes >= maxQueuedBytes)
        return;

    if (!SDL_PutAudioStreamData(stream,
                                pendingSamples.data(),
                                static_cast<int>(pendingSamples.size() * sizeof(int16_t))))
    {
        SDL_Log("SDL_PutAudioStreamData failed: %s", SDL_GetError());
    }

    pendingSamples.clear();
}
