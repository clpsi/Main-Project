#include "Noise_Generator.h"


// ============================================================================
// GLOBAL SEED
// ============================================================================

int32 FNoise_Generator::Seed = 0;


// ============================================================================
// INITIALIZATION
// ============================================================================

void FNoise_Generator::Initialize(
    int32 InSeed
)
{
    Seed = InSeed;
}


// ============================================================================
// 32-BIT HASH
// ============================================================================

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


// ============================================================================
// 2D HASH
// ============================================================================

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


    H = static_cast<uint32>(H);


    H ^= static_cast<uint32>(Y)
        * 0x85EBCA6Bu;


    H = static_cast<uint32>(H);


    return Hash32(H);
}


// ============================================================================
// HASH -> [-1, +1]
// ============================================================================

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


// ============================================================================
// FADE
// ============================================================================

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


// ============================================================================
// LERP
// ============================================================================

float FNoise_Generator::Lerp(
    float A,
    float B,
    float T
)
{
    return A + (B - A) * T;
}


// ============================================================================
// CLAMP
// ============================================================================

float FNoise_Generator::Clamp(
    float Value,
    float Minimum,
    float Maximum
)
{
    return FMath::Max(
        Minimum,
        FMath::Min(
            Maximum,
            Value
        )
    );
}


// ============================================================================
// REMAP
// ============================================================================

float FNoise_Generator::Remap(
    float Value,
    float OldMin,
    float OldMax,
    float NewMin,
    float NewMax
)
{
    if (OldMax == OldMin)
    {
        return NewMin;
    }


    float T =
        (
            Value - OldMin
            )
        /
        (
            OldMax - OldMin
            );


    T = Clamp(
        T,
        0.0f,
        1.0f
    );


    return
        NewMin
        +
        (
            NewMax - NewMin
            )
        * T;
}


// ============================================================================
// POSITIVE MODULO
// ============================================================================
//
// Reproduces Python's:
//
//     Value % Period
//
// for positive Period values.
//
// ============================================================================

int32 FNoise_Generator::PositiveModulo(
    int32 Value,
    int32 Period
)
{
    if (Period <= 0)
    {
        return 0;
    }


    const int32 Result =
        Value % Period;


    if (Result < 0)
    {
        return Result + Period;
    }


    return Result;
}


// ============================================================================
// PERIODIC VALUE NOISE
// ============================================================================

float FNoise_Generator::ValueNoise2D(
    float X,
    float Y,
    int32 InSeed,
    int32 PeriodX,
    int32 PeriodY
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


    // ========================================================================
    // WRAP LATTICE COORDINATES
    // ========================================================================

    const int32 X0Hash =
        PositiveModulo(
            X0,
            PeriodX
        );


    const int32 X1Hash =
        PositiveModulo(
            X1,
            PeriodX
        );


    const int32 Y0Hash =
        PositiveModulo(
            Y0,
            PeriodY
        );


    const int32 Y1Hash =
        PositiveModulo(
            Y1,
            PeriodY
        );


    // ========================================================================
    // HASH CORNERS
    // ========================================================================

    const float V00 =
        Hash2DNormalized(
            X0Hash,
            Y0Hash,
            InSeed
        );


    const float V10 =
        Hash2DNormalized(
            X1Hash,
            Y0Hash,
            InSeed
        );


    const float V01 =
        Hash2DNormalized(
            X0Hash,
            Y1Hash,
            InSeed
        );


    const float V11 =
        Hash2DNormalized(
            X1Hash,
            Y1Hash,
            InSeed
        );


    // ========================================================================
    // INTERPOLATION
    // ========================================================================

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


// ============================================================================
// FBM
// ============================================================================

float FNoise_Generator::FBM2D(
    float X,
    float Y,
    int32 InSeed,
    int32 BaseCells,
    int32 Octaves
)
{
    float Total = 0.0f;

    float Amplitude = 1.0f;

    float AmplitudeSum = 0.0f;


    for (int32 Octave = 0;
        Octave < Octaves;
        ++Octave)
    {
        // ====================================================================
        // OCTAVE SEED
        // ====================================================================

        const int32 OctaveSeed =
            InSeed
            + Octave * 1013;


        // ====================================================================
        // FREQUENCY
        // ====================================================================

        /*
         * Lacunarity is fixed at 2.
         *
         * Therefore:
         *
         *     octave 0 -> 1
         *     octave 1 -> 2
         *     octave 2 -> 4
         *     octave 3 -> 8
         */
        const int32 Frequency =
            1 << Octave;


        // ====================================================================
        // PERIOD
        // ====================================================================

        /*
         * Base cells also double every octave.
         */
        const int32 Period =
            BaseCells * Frequency;


        // ====================================================================
        // NOISE
        // ====================================================================

        Total +=
            ValueNoise2D(
                X * static_cast<float>(Frequency),
                Y * static_cast<float>(Frequency),

                OctaveSeed,

                Period,
                Period
            )
            * Amplitude;


        AmplitudeSum +=
            Amplitude;


        Amplitude *=
            TerrainConfig::NoisePersistence;
    }


    // ========================================================================
    // NORMALIZATION
    // ========================================================================

    if (AmplitudeSum > 0.0f)
    {
        Total /= AmplitudeSum;
    }


    return Total;
}


// ============================================================================
// RIDGED FBM
// ============================================================================

float FNoise_Generator::RidgedFBM2D(
    float X,
    float Y,
    int32 InSeed,
    int32 BaseCells,
    int32 Octaves,
    float Sharpness
)
{
    float Total = 0.0f;

    float Amplitude = 1.0f;

    float AmplitudeSum = 0.0f;


    for (int32 Octave = 0;
        Octave < Octaves;
        ++Octave)
    {
        // ====================================================================
        // OCTAVE SEED
        // ====================================================================

        const int32 OctaveSeed =
            InSeed
            + 5000
            + Octave * 1013;


        // ====================================================================
        // FREQUENCY
        // ====================================================================

        const int32 Frequency =
            1 << Octave;


        // ====================================================================
        // PERIOD
        // ====================================================================

        const int32 Period =
            BaseCells * Frequency;


        // ====================================================================
        // NOISE
        // ====================================================================

        const float Noise =
            ValueNoise2D(
                X * static_cast<float>(Frequency),
                Y * static_cast<float>(Frequency),

                OctaveSeed,

                Period,
                Period
            );


        // ====================================================================
        // RIDGE
        // ====================================================================

        float Ridge =
            1.0f
            - FMath::Abs(
                Noise
            );


        Ridge =
            FMath::Pow(
                Ridge,
                Sharpness
            );


        // ====================================================================
        // ACCUMULATE
        // ====================================================================

        Total +=
            Ridge
            * Amplitude;


        AmplitudeSum +=
            Amplitude;


        Amplitude *=
            TerrainConfig::NoisePersistence;
    }


    // ========================================================================
    // NORMALIZATION
    // ========================================================================

    if (AmplitudeSum > 0.0f)
    {
        Total /= AmplitudeSum;
    }


    return Total;
}


// ============================================================================
// CONTINENTALNESS
// ============================================================================

float FNoise_Generator::GetContinentalness(
    float WorldX,
    float WorldY
)
{
    const float X =
        (
            WorldX
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::ContinentCells;


    const float Y =
        (
            WorldY
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::ContinentCells;


    return FBM2D(
        X,
        Y,

        Seed + 10000,

        TerrainConfig::ContinentCells,
        TerrainConfig::ContinentOctaves
    );
}


// ============================================================================
// EROSION
// ============================================================================

float FNoise_Generator::GetErosion(
    float WorldX,
    float WorldY
)
{
    const float X =
        (
            WorldX
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::ErosionCells;


    const float Y =
        (
            WorldY
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::ErosionCells;


    return FBM2D(
        X,
        Y,

        Seed + 20000,

        TerrainConfig::ErosionCells,
        TerrainConfig::ErosionOctaves
    );
}


// ============================================================================
// MOUNTAINS
// ============================================================================

float FNoise_Generator::GetMountains(
    float WorldX,
    float WorldY
)
{
    const float X =
        (
            WorldX
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::MountainCells;


    const float Y =
        (
            WorldY
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::MountainCells;


    return RidgedFBM2D(
        X,
        Y,

        Seed + 30000,

        TerrainConfig::MountainCells,
        TerrainConfig::MountainOctaves,

        TerrainConfig::MountainSharpness
    );
}


// ============================================================================
// VALLEYS
// ============================================================================

float FNoise_Generator::GetValleys(
    float WorldX,
    float WorldY
)
{
    const float X =
        (
            WorldX
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::ValleyCells;


    const float Y =
        (
            WorldY
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::ValleyCells;


    return RidgedFBM2D(
        X,
        Y,

        Seed + 35000,

        TerrainConfig::ValleyCells,
        TerrainConfig::ValleyOctaves,

        TerrainConfig::ValleySharpness
    );
}


// ============================================================================
// DETAIL
// ============================================================================

float FNoise_Generator::GetDetail(
    float WorldX,
    float WorldY
)
{
    const float X =
        (
            WorldX
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::DetailCells;


    const float Y =
        (
            WorldY
            /
            TerrainConfig::WorldRepeatSize
            )
        *
        TerrainConfig::DetailCells;


    return FBM2D(
        X,
        Y,

        Seed + 40000,

        TerrainConfig::DetailCells,
        TerrainConfig::DetailOctaves
    );
}


// ============================================================================
// TERRAIN HEIGHT
// ============================================================================

float FNoise_Generator::GetHeight(
    float WorldX,
    float WorldY
)
{
    // ========================================================================
    // CONTINENTALNESS
    // ========================================================================

    const float Continent =
        GetContinentalness(
            WorldX,
            WorldY
        );


    const float Continent01 =
        Continent * 0.5f
        + 0.5f;


    // ========================================================================
    // BASE TERRAIN
    // ========================================================================

    const float Base =
        Remap(
            Continent01,

            TerrainConfig::ContinentLandStart,
            TerrainConfig::ContinentLandEnd,

            TerrainConfig::ContinentMinHeight,
            TerrainConfig::ContinentMaxHeight
        );


    // ========================================================================
    // EROSION / RUGGEDNESS
    // ========================================================================

    const float Erosion =
        GetErosion(
            WorldX,
            WorldY
        );


    const float Erosion01 =
        Erosion * 0.5f
        + 0.5f;


    const float Ruggedness =
        Remap(
            Erosion01,

            0.0f,
            1.0f,

            TerrainConfig::ErosionMinRuggedness,
            TerrainConfig::ErosionMaxRuggedness
        );


    // ========================================================================
    // MOUNTAINS
    // ========================================================================

    const float Mountains =
        GetMountains(
            WorldX,
            WorldY
        );


    float MountainMask =
        Remap(
            Continent01,

            TerrainConfig::MountainLandStart,
            TerrainConfig::MountainLandEnd,

            0.0f,
            1.0f
        );


    MountainMask =
        Clamp(
            MountainMask,
            0.0f,
            1.0f
        );


    const float MountainHeight =
        Mountains
        * MountainMask
        * Ruggedness
        * TerrainConfig::MountainHeight;


    // ========================================================================
    // VALLEY LAND MASK
    // ========================================================================

    float ValleyLandMask =
        Remap(
            Continent01,

            TerrainConfig::ValleyLandStart,
            TerrainConfig::ValleyLandEnd,

            0.0f,
            1.0f
        );


    ValleyLandMask =
        Clamp(
            ValleyLandMask,
            0.0f,
            1.0f
        );


    // ========================================================================
    // VALLEY RUGGEDNESS MASK
    // ========================================================================

    float ValleyRuggednessMask =
        Remap(
            Ruggedness,

            TerrainConfig::ValleyRuggednessStart,
            TerrainConfig::ValleyRuggednessEnd,

            0.0f,
            1.0f
        );


    ValleyRuggednessMask =
        Clamp(
            ValleyRuggednessMask,
            0.0f,
            1.0f
        );


    // ========================================================================
    // VALLEY MASK
    // ========================================================================

    const float ValleyMask =
        ValleyLandMask
        * MountainMask
        * ValleyRuggednessMask;


    // ========================================================================
    // VALLEY NOISE
    // ========================================================================

    const float Valleys =
        GetValleys(
            WorldX,
            WorldY
        );


    // ========================================================================
    // VALLEY CARVING
    // ========================================================================

    const float ValleyCarve =
        Valleys
        * ValleyMask
        * TerrainConfig::ValleyDepth;


    // ========================================================================
    // DETAIL
    // ========================================================================

    const float Detail =
        GetDetail(
            WorldX,
            WorldY
        );


    const float DetailHeight =
        Detail
        * TerrainConfig::DetailHeight;


    // ========================================================================
    // FINAL HEIGHT
    // ========================================================================

    const float Height =
        Base
        + MountainHeight
        - ValleyCarve
        + DetailHeight;


    return Height
        * TerrainConfig::HeightScale;
}
