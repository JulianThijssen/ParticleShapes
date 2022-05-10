#pragma once

#include <vector>

#define TWO_PI 6.28318530718

float cosineWave(float frequency, float t);

float sineWave(float frequency, float t);

float bpm(float baseFrequency, float bpm, float t);

float drum(float frequency, float t);

float customWave(std::vector<float>& frequencies, float t);
