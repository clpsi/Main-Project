#include "Noise_Generator.h"


int32 FNoise_Generator::Seed = 0;

void FNoise_Generator::Initialize(
    int32 InSeed
)
{
    Seed = InSeed;
}
/*
 * ---------------------------------------------------------
 * 32-bit deterministic hash
 * ---------------------------------------------------------
 *
 * This is deliberately independent from Unreal's random
 * number generator.
 *
 * Same input -> same output.
 */
uint32 FNoise_Generator::Hash32(
    uint32 X
)
{
    X ^= X >> 16;
    X *= 0x7FEB352Du;

    X ^= X >> 15;
    X *= 0x846CA68Bu;

    X ^= X >> 16;

    return X;
}


/*
 * ---------------------------------------------------------
 * 2D hash
 * ---------------------------------------------------------
 */
uint32 FNoise_Generator::Hash2D(
    int32 X,
    int32 Y,
    int32 InSeed
)
{
    uint32 H =
        static_cast<uint32>(InSeed);

    H ^= static_cast<uint32>(X)
        * 0x9E3779B9u;

    H ^= static_cast<uint32>(Y)
        * 0x85EBCA6Bu;

    return Hash32(H);
}


/*
 * ---------------------------------------------------------
 * Hash -> [-1, +1]
 * ---------------------------------------------------------
 */
float FNoise_Generator::Hash2DNormalized(
    int32 X,
    int32 Y,
    int32 InSeed
)
{
    const uint32 Hash =
        Hash2D(
            X,
            Y,
            InSeed
        );


    const float Value =
        static_cast<float>(Hash)
        / 4294967295.0f;


    return Value * 2.0f - 1.0f;
}


/*
 * ---------------------------------------------------------
 * Quintic smoothing curve
 * ---------------------------------------------------------
 *
 * This produces much smoother terrain than linear
 * interpolation.
 */
float FNoise_Generator::Fade(
    float T
)
{
    return
        T * T * T *
        (
            T * (
                T * 6.0f
                - 15.0f
                )
            + 10.0f
            );
}


/*
 * ---------------------------------------------------------
 * Linear interpolation
 * ---------------------------------------------------------
 */
float FNoise_Generator::Lerp(
    float A,
    float B,
    float T
)
{
    return A + (B - A) * T;
}


/*
 * ---------------------------------------------------------
 * 2D Value Noise
 * ---------------------------------------------------------
 */
float FNoise_Generator::ValueNoise2D(
    float X,
    float Y,
    int32 InSeed
)
{
    const int32 X0 =
        FMath::FloorToInt(X);

    const int32 Y0 =
        FMath::FloorToInt(Y);


    const int32 X1 =
        X0 + 1;

    const int32 Y1 =
        Y0 + 1;


    const float FX =
        X - static_cast<float>(X0);

    const float FY =
        Y - static_cast<float>(Y0);


    const float SX =
        Fade(FX);

    const float SY =
        Fade(FY);


    const float V00 =
        Hash2DNormalized(
            X0,
            Y0,
            InSeed
        );

    const float V10 =
        Hash2DNormalized(
            X1,
            Y0,
            InSeed
        );

    const float V01 =
        Hash2DNormalized(
            X0,
            Y1,
            InSeed
        );

    const float V11 =
        Hash2DNormalized(
            X1,
            Y1,
            InSeed
        );


    const float Bottom =
        Lerp(
            V00,
            V10,
            SX
        );


    const float Top =
        Lerp(
            V01,
            V11,
            SX
        );


    return Lerp(
        Bottom,
        Top,
        SY
    );
}


/*
 * ---------------------------------------------------------
 * Fractal Brownian Motion
 * ---------------------------------------------------------
 */
float FNoise_Generator::FBM2D(
    float X,
    float Y,
    int32 InSeed,
    int32 Octaves,
    float Lacunarity,
    float Persistence
)
{
    float Sum = 0.0f;

    float Amplitude = 1.0f;
    float Frequency = 1.0f;

    float AmplitudeSum = 0.0f;


    for (int32 I = 0; I < Octaves; ++I)
    {
        /*
         * Every octave gets a different deterministic
         * seed.
         */
        const int32 OctaveSeed =
            InSeed + I * 1013;


        Sum +=
            ValueNoise2D(
                X * Frequency,
                Y * Frequency,
                OctaveSeed
            )
            * Amplitude;


        AmplitudeSum += Amplitude;


        Frequency *= Lacunarity;

        Amplitude *= Persistence;
    }


    /*
     * Normalize the accumulated amplitudes so that
     * changing the number of octaves doesn't cause
     * the terrain height to explode.
     */
    if (AmplitudeSum > 0.0f)
    {
        Sum /= AmplitudeSum;
    }


    return Sum;
}


/*
 * ---------------------------------------------------------
 * Terrain Height
 * ---------------------------------------------------------
 */
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
    const float NoiseX =
        WorldX * Frequency;

    const float NoiseY =
        WorldY * Frequency;


    const float Noise =
        FBM2D(
            NoiseX,
            NoiseY,

            Seed,

            Octaves,
            Lacunarity,
            Persistence
        );


    return Noise * HeightScale;
}