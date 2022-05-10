#pragma once

#include "soloud.h"

#include <vector>

class CustomAudioSource : public SoLoud::AudioSource
{
public:
    CustomAudioSource();
    virtual ~CustomAudioSource();
    virtual SoLoud::AudioSourceInstance* createInstance();

    std::vector<float> mSamples;
};

class CustomAudioSourceInstance : public SoLoud::AudioSourceInstance
{
public:
    virtual unsigned int getAudio(float* aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize);
    virtual bool hasEnded();
    //virtual SoLoud::result seek(float aSeconds, float* mScratch, int mScratchSize);
    //virtual SoLoud::result rewind();

private:
    int mSamplesRead = 0;
};
