
#include "WeatherNoiseShader.h"

IMPLEMENT_GLOBAL_SHADER(
    FWeatherNoiseCS,
    "/Project/Private/WeatherNoise.usf",
    "MainCS",
    SF_Compute
);
