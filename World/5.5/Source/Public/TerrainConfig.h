#pragma once

#include "CoreMinimal.h"


// ============================================================================
// TERRAIN CONFIGURATION
// ============================================================================
//
// Everything in this namespace defines the mathematical terrain.
//
// These values are intentionally compile-time constants.
//
// Runtime systems should NOT pass these parameters around.
//
// The terrain is deterministic from:
//
//     Seed
//     WorldX
//     WorldY
//
// ============================================================================

namespace TerrainConfig
{
    // ========================================================================
    // WORLD
    // ========================================================================

    /*
     * Complete physical repeat distance of the terrain.
     *
     * The terrain repeats every 25.6 km in both X and Y.
     */
    constexpr float WorldRepeatSize = 25600.0f;


    // ========================================================================
    // GLOBAL HEIGHT
    // ========================================================================

    constexpr float HeightScale = 2000.0f;


    // ========================================================================
    // NOISE SYSTEM
    // ========================================================================

    /*
     * These are system-wide constants.
     *
     * Every octave:
     *
     *     frequency *= 2
     *     amplitude *= 0.5
     */
    constexpr float NoiseLacunarity = 2.0f;

    constexpr float NoisePersistence = 0.5f;


    // ========================================================================
    // CONTINENTALNESS
    // ========================================================================

    constexpr int32 ContinentCells = 8;
    constexpr int32 ContinentOctaves = 4;

    constexpr float ContinentLandStart = 0.30f;
    constexpr float ContinentLandEnd = 0.70f;

    constexpr float ContinentMinHeight = -0.25f;
    constexpr float ContinentMaxHeight = 0.45f;


    // ========================================================================
    // EROSION / RUGGEDNESS
    // ========================================================================

    constexpr int32 ErosionCells = 38;
    constexpr int32 ErosionOctaves = 4;

    constexpr float ErosionMinRuggedness = 0.35f;
    constexpr float ErosionMaxRuggedness = 1.0f;


    // ========================================================================
    // MOUNTAINS
    // ========================================================================

    constexpr int32 MountainCells = 20;
    constexpr int32 MountainOctaves = 5;

    constexpr float MountainHeight = 0.9f;

    constexpr float MountainLandStart = 0.45f;
    constexpr float MountainLandEnd = 0.65f;

    constexpr float MountainSharpness = 2.0f;


    // ========================================================================
    // VALLEYS
    // ========================================================================

    constexpr int32 ValleyCells = 9;
    constexpr int32 ValleyOctaves = 4;

    constexpr float ValleyDepth = 0.35f;

    constexpr float ValleySharpness = 1.5f;

    constexpr float ValleyLandStart = 0.40f;
    constexpr float ValleyLandEnd = 0.65f;

    constexpr float ValleyRuggednessStart = 0.25f;
    constexpr float ValleyRuggednessEnd = 1.0f;


    // ========================================================================
    // DETAIL
    // ========================================================================

    constexpr int32 DetailCells = 102;
    constexpr int32 DetailOctaves = 3;

    constexpr float DetailHeight = 0.01f;


    // ========================================================================
    // SEA LEVEL
    // ========================================================================

    constexpr float SeaLevel = 0.0f;
}
