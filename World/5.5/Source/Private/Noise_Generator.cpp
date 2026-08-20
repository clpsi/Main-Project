#include "Noise_Generator.h"

#include "SimplexNoiseBPLibrary.h"


void FNoise_Generator::Initialize(
    int32 Seed
)
{
    USimplexNoiseBPLibrary::setNoiseSeed(
        Seed
    );
}

float FNoise_Generator::GetHeight(
    float WorldX,
    float WorldY,

    float Frequency,
    int32 Octaves,
    float Lacunarity,
    float Persistence,
    float HeightScale
)
{

    /*
     * Convert world coordinates into noise space.
     *
     * Frequency controls the size of terrain features.
     */
    const float SampleX =
        WorldX * Frequency;


    const float SampleY =
        WorldY * Frequency;


    /*
     * Generate fractal Simplex noise.
     *
     * ZeroToOne = false:
     *
     *     approximately -1 ... +1
     */
    const float Noise =
        USimplexNoiseBPLibrary::GetSimplexNoise2D_EX(
            SampleX,
            SampleY,

            Lacunarity,
            Persistence,
            Octaves,

            1.0f,

            false
        );


    /*
     * Convert noise into Unreal units.
     */
    return Noise * HeightScale;
}
