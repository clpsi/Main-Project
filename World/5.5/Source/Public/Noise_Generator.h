// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class LANDSCAPE_PROJ_API FNoise_Generator
{
public:

    /**
    * Initializing once
    **/
    static void Initialize(int32 InSeed);

    /**
     * Calculate terrain height at world-space X/Y coordinates.
     */
    static float GetHeight(
        float WorldX,
        float WorldY,

        float Frequency,
        int32 Octaves,
        float Lacunarity,
        float Persistence,
        float HeightScale
    );


private:

    static int32 Seed;


    /*
     * Deterministic 32-bit hash.
     */
    static uint32 Hash32(uint32 X);


    /*
     * Convert X/Y/Seed into deterministic random value.
     */
    static uint32 Hash2D(
        int32 X,
        int32 Y,
        int32 Seed
    );


    /*
     * Hash result -> [-1, 1].
     */
    static float Hash2DNormalized(
        int32 X,
        int32 Y,
        int32 Seed
    );


    /*
     * Quintic interpolation.
     */
    static float Fade(float T);


    /*
     * Linear interpolation.
     */
    static float Lerp(
        float A,
        float B,
        float T
    );


    /*
     * 2D value noise.
     */
    static float ValueNoise2D(
        float X,
        float Y,
        int32 Seed
    );


    /*
     * Fractal Brownian Motion.
     */
    static float FBM2D(
        float X,
        float Y,
        int32 Seed,
        int32 Octaves,
        float Lacunarity,
        float Persistence
    );
};
