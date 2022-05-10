#include "Sound.h"

#include <cmath>

float cosineWave(float frequency, float t)
{
    return cos(TWO_PI * frequency * t);
}

float sineWave(float frequency, float t)
{
    return sin(TWO_PI * frequency * t);
}

float drum(float frequency, float t)
{
    return sin(TWO_PI * frequency * t) + sin(TWO_PI * (frequency + 100) * t) * 0.5f + sin(TWO_PI * (frequency + 200) * t) * 0.25f;
}

float customWave(std::vector<float>& frequencies, float t)
{
    float sample = 0;

    for (int freq = 0; freq < frequencies.size(); freq++)
    {
        sample += sineWave(freq, t) * frequencies[freq];
    }
    return sample;
}

template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

float bpm(float baseFrequency, float bpm, float t)
{
    float a = (1 - (60 / bpm)) * 4; // This will break if bpm lower than 60 probs FIXME // Why does it need to be * 4?
    float b = bpm / 60; // Alternative calculation which is a * 4
    return drum(baseFrequency, t) + drum(baseFrequency + b, t);
}
