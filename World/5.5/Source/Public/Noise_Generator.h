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
    static void Initialize(
        int32 Seed
    );

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

    /**
     * Main procedural terrain signal.
     */
    static float GetTerrainNoise(
        float WorldX,
        float WorldY
    );
};
