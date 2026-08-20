#pragma once

#include "CoreMinimal.h"

#include <atomic>


/*
 * Called on the Game Thread after generation has finished.
 *
 * bWasCancelled tells the owner whether the generation completed
 * normally or was cancelled.
 */
using FChunkGenerationComplete =
TFunction<void(bool bWasCancelled)>;


class Chunk_Generator
    : public TSharedFromThis<Chunk_Generator>
{
public:

    Chunk_Generator(
        int32 InResolution,
        float InChunkSize,
        const FVector& InChunkLocation
    );

    ~Chunk_Generator();


    /*
     * Starts asynchronous generation.
     *
     * CompletionCallback is always executed on the Game Thread.
     */
    void GenerateAsync(
        FChunkGenerationComplete CompletionCallback
    );


    /*
     * Requests cancellation.
     *
     * The worker exits at its next cancellation check.
     */
    void Cancel();


    bool IsCancelled() const;
    bool IsFinished() const;


public:

    // ============================================================
    // GENERATED RENDER DATA
    // ============================================================

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;


    // ============================================================
    // GENERATED COLLISION DATA
    // ============================================================

    TArray<FVector> CollisionVertices;
    TArray<int32> CollisionTriangles;


private:

    // ============================================================
    // GENERATION
    // ============================================================

    void GenerateVertices();
    void GenerateTriangles();
    void GenerateNormals();
    void GenerateUVs();
    void GenerateCollision();


    bool ShouldStop() const;


private:

    // ============================================================
    // PARAMETERS
    // ============================================================

    int32 Resolution = 0;

    float ChunkSize = 0.0f;

    FVector ChunkLocation = FVector::ZeroVector;


    // ============================================================
    // ASYNC STATE
    // ============================================================

    std::atomic<bool> bCancelRequested{ false };

    std::atomic<bool> bFinished{ false };

    std::atomic<bool> bStarted{ false };
};
