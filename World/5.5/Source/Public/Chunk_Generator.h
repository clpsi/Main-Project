#pragma once

#include "CoreMinimal.h"

#include <atomic>
#include <memory>


// ============================================================================
// COMPLETION CALLBACK
// ============================================================================

using FChunkGenerationComplete =
TFunction<void(bool bWasCancelled)>;


// ============================================================================
// CHUNK GENERATOR
// ============================================================================

class Chunk_Generator
    : public TSharedFromThis<Chunk_Generator>
{
public:

    Chunk_Generator(
        int32 InResolution,
        float InChunkSize,
        int32 InChunkX,
        int32 InChunkY
    );

    ~Chunk_Generator();


    // ========================================================================
    // ASYNC GENERATION
    // ========================================================================

    void GenerateAsync(
        FChunkGenerationComplete CompletionCallback
    );


    // ========================================================================
    // CANCELLATION
    // ========================================================================

    void Cancel();

    bool IsCancelled() const;

    bool IsFinished() const;


public:

    // ========================================================================
    // GENERATED DATA
    // ========================================================================

    TArray<FVector> Vertices;

    TArray<int32> Triangles;

    TArray<FVector> Normals;

    TArray<FVector2D> UV0;


private:

    // ========================================================================
    // GENERATION
    // ========================================================================

    void GenerateVertices();

    void GenerateTriangles();

    void GenerateNormals();


    // ========================================================================
    // STATE
    // ========================================================================

    bool ShouldStop() const;


private:

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    int32 Resolution = 0;

    float ChunkSize = 0.0f;

    FIntPoint ChunkCoordinate;


    // ========================================================================
    // STATE
    // ========================================================================

    std::atomic<bool> bCancelRequested{ false };

    std::atomic<bool> bStarted{ false };

    std::atomic<bool> bFinished{ false };
};
