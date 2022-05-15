#include "CustomAudioSource.h"

#include "Sound.h"

#include <fftw3.h>

#include <iostream>
#include <random>

std::vector<float> testFrequencies;
std::vector<float> testInput;

#define N 44100

namespace
{
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0, 1);
}

void reverseFrequencies(std::vector<float> frequencies)
{
    fftw_complex in[N], out[N], in2[N]; /* double [2] */
    fftw_plan p, q;
    int i;

    /* prepare a cosine wave */
    for (i = 0; i < N; i++) {
        in[i][0] = cos(100 * TWO_PI * ((float)i / N)) * 0.1f;
        in[i][1] = 0;
    }

    /* forward Fourier transform, save the result in 'out' */
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);

    for (i = 0; i < N; i++)
        printf("freq: %3d %+9.5f %+9.5f I\n", i, out[i][0], out[i][1]);
    fftw_destroy_plan(p);


    //for (int i = 0; i < N; i++)
    //{
    //    out[i][0] = frequencies[i];
    //    out[i][1] = 0;
    //}

    /* backward Fourier transform, save the result in 'in2' */
    printf("\nInverse transform:\n");
    q = fftw_plan_dft_1d(N, out, in2, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(q);
    /* normalize */
    for (int i = 0; i < N; i++) {
        in2[i][0] *= 1. / N;
        in2[i][1] *= 1. / N;
    }
    for (int i = 0; i < N; i++)
    {
        printf("recover: %3d %+9.5f %+9.5f I vs. %+9.5f %+9.5f I\n", i, 0, 0, in2[i][0], in2[i][1]);
        testInput[i] = in2[i][0];
    }
    fftw_destroy_plan(q);

    fftw_cleanup();
}

void generateWhiteNoise()
{
    fftw_complex* freq = fftw_alloc_complex(N);
    fftw_complex* out = fftw_alloc_complex(N);

    fftw_plan plan;
    plan = fftw_plan_dft_1d(N, freq, out, FFTW_BACKWARD, FFTW_ESTIMATE);

    // Set frequencies for white noise
    for (int i = 0; i < N; i++)
    {
        freq[i][0] = 0;
        freq[i][1] = 0;
    }

    int maxFreq = 600;
    for (int i = 100; i < maxFreq; i++)
    {
        float phase = distribution(generator) * TWO_PI;
        float dMag = 400.0f;
        float magnitude = dMag - (dMag * i / maxFreq);
        freq[i][0] = cos(phase) * magnitude;
        freq[i][1] = sin(phase) * magnitude;

        //freq[N - i][0] = -freq[i][0];
        //freq[N - i][1] = -freq[i][1];
    }

    /* backward Fourier transform, save the result in 'in2' */
    printf("\nInverse transform:\n");
    fftw_execute(plan);

    /* normalize */
    for (int i = 0; i < N; i++) {
        out[i][0] *= 1. / N;
        out[i][1] *= 1. / N;
    }
    for (int i = 0; i < N; i++)
    {
        printf("recover: %3d %+9.5f %+9.5f I vs. %+9.5f %+9.5f I\n", i, 0, 0, out[i][0], out[i][1]);
        testInput[i] = out[i][0];
    }
    fftw_destroy_plan(plan);

    fftw_cleanup();
}

CustomAudioSource::CustomAudioSource()
{
    mChannels = 1;
    mBaseSamplerate = 44100;

    mSamples.resize(1000000, 0);

    testFrequencies.resize(N);
    testInput.resize(N);
    for (int i = 0; i < testFrequencies.size(); i++)
    {
        //if (i < 120) testFrequencies[i] = abs(i - 0) / 10000.0f;
        //else if (i < 1000) testFrequencies[i] = (120 - (120.0f/880) * abs(i - 120)) / 10000.0f;
        //else testFrequencies[i] = 0;
        testFrequencies[i] = 0;
        if (i == 200)
            testFrequencies[i] = 25;
    }

    //reverseFrequencies(testFrequencies);
    generateWhiteNoise();
}

CustomAudioSource::~CustomAudioSource()
{

}

SoLoud::AudioSourceInstance* CustomAudioSource::createInstance()
{
    return new CustomAudioSourceInstance();
}

int t = 0;

unsigned int CustomAudioSourceInstance::getAudio(float* aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize)
{
    t += 1;

    //if (t % 10 == 0)
    //{
    //    for (int i = 0; i < testFrequencies.size(); i++)
    //    {
    //        testFrequencies[i] = 0.0001f;
    //    }
    //}

    for (int i = 0; i < aSamplesToRead; i++)
    {
        float fracInSecond = (mSamplesRead + i) / mBaseSamplerate;
        if (fracInSecond >= 1)
            fracInSecond -= (int)fracInSecond;

        // customWave(testFrequencies, fracInSecond);// bpm(100, 126, fracInSecond);// (sineWave(73.42f, fracInSecond) + sineWave(73.78f, fracInSecond)) * 0.5f;
        //aBuffer[i] = cosineWave(200, fracInSecond);
        aBuffer[i] = testInput[(int) (testInput.size() * fracInSecond)];
    }
    //std::cout << aSamplesToRead << " " << aBufferSize << std::endl;
    mSamplesRead += aSamplesToRead;
    return aSamplesToRead;
}

bool CustomAudioSourceInstance::hasEnded()
{
    return mSamplesRead > 1000000;
}
//
//SoLoud::result CustomAudioSourceInstance::seek(float aSeconds, float* mScratch, int mScratchSize)
//{
//
//}

//SoLoud::result CustomAudioSourceInstance::rewind()
//{
//
//}
