#pragma once

#include "CoreMinimal.h"
#include "TerrainConfig.h"


// ============================================================================
// NOISE GENERATOR
// ============================================================================
//
// Deterministic procedural terrain.
//
// Public interface:
//
//     Initialize(Seed)
//     GetHeight(WorldX, WorldY)
//
// Everything else is implementation detail.
//
// ============================================================================

class LANDSCAPE_PROJ_API FNoise_Generator
{
public:

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /*
     * Sets the global terrain seed.
     *
     * The intended usage is to call this once before terrain generation
     * begins and then leave the seed unchanged for the lifetime of the world.
     */
    static void Initialize(
        int32 InSeed
    );


    // ========================================================================
    // TERRAIN
    // ========================================================================

    /*
     * Returns the final terrain height at world coordinates.
     *
     * The terrain is periodic with TerrainConfig::WorldRepeatSize.
     */
    static float GetHeight(
        float WorldX,
        float WorldY
    );


private:

    // ========================================================================
    // HASH
    // ========================================================================

    static uint32 Hash32(
        uint32 X
    );


    static uint32 Hash2D(
        int32 X,
        int32 Y,
        int32 InSeed
    );


    static float Hash2DNormalized(
        int32 X,
        int32 Y,
        int32 InSeed
    );


    // ========================================================================
    // MATH
    // ========================================================================

    static float Fade(
        float T
    );


    static float Lerp(
        float A,
        float B,
        float T
    );


    static float Clamp(
        float Value,
        float Minimum,
        float Maximum
    );


    static float Remap(
        float Value,
        float OldMin,
        float OldMax,
        float NewMin,
        float NewMax
    );


    // ========================================================================
    // PERIODIC COORDINATE
    // ========================================================================

    /*
     * C++ modulo behaves differently from Python for negative values.
     *
     * Python:
     *
     *     -1 % 20 == 19
     *
     * C++:
     *
     *     -1 % 20 == -1
     *
     * This helper reproduces Python's positive modulo behaviour.
     */
    static int32 PositiveModulo(
        int32 Value,
        int32 Period
    );


    // ========================================================================
    // VALUE NOISE
    // ========================================================================

    static float ValueNoise2D(
        float X,
        float Y,
        int32 InSeed,
        int32 PeriodX,
        int32 PeriodY
    );


    // ========================================================================
    // FBM
    // ========================================================================

    static float FBM2D(
        float X,
        float Y,
        int32 InSeed,
        int32 BaseCells,
        int32 Octaves
    );


    // ========================================================================
    // RIDGED FBM
    // ========================================================================

    static float RidgedFBM2D(
        float X,
        float Y,
        int32 InSeed,
        int32 BaseCells,
        int32 Octaves,
        float Sharpness
    );


    // ========================================================================
    // TERRAIN LAYERS
    // ========================================================================

    static float GetContinentalness(
        float WorldX,
        float WorldY
    );


    static float GetErosion(
        float WorldX,
        float WorldY
    );


    static float GetMountains(
        float WorldX,
        float WorldY
    );


    static float GetValleys(
        float WorldX,
        float WorldY
    );


    static float GetDetail(
        float WorldX,
        float WorldY
    );


private:

    // ========================================================================
    // GLOBAL SEED
    // ========================================================================

    /*
     * Global deterministic terrain seed.
     *
     * This is intentionally not passed through every terrain function.
     */
    static int32 Seed;
};
