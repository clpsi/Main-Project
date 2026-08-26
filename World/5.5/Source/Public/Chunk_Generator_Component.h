#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Chunk_Generator_Component.generated.h"


class Chunk_Generator;


// ============================================================================
// BLUEPRINT EVENT
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(
    FOnChunkGenerated,

    int32,
    ChunkX,

    int32,
    ChunkY,

    const TArray<FVector>&,
    Vertices,

    const TArray<int32>&,
    Triangles,

    const TArray<FVector>&,
    Normals,

    const TArray<FVector2D>&,
    UV0
);



// ============================================================================
// COMPONENT
// ============================================================================

UCLASS(
    ClassGroup = (ProceduralTerrain),
    meta = (BlueprintSpawnableComponent)
)
class LANDSCAPE_PROJ_API UChunk_Generator_Component
    : public UActorComponent
{
    GENERATED_BODY()


public:

    UChunk_Generator_Component();


protected:

    virtual void BeginPlay() override;


    virtual void EndPlay(
        const EEndPlayReason::Type EndPlayReason
    ) override;


public:

    // ========================================================================
    // GENERATION
    // ========================================================================

    UFUNCTION(
        BlueprintCallable,
        Category = "Chunk Generation"
    )
    void GenerateChunk(
        int32 ChunkX,
        int32 ChunkY
    );


    // ========================================================================
    // CANCELLATION
    // ========================================================================

    UFUNCTION(
        BlueprintCallable,
        Category = "Chunk Generation"
    )
    void CancelChunk(
        int32 ChunkX,
        int32 ChunkY
    );


    UFUNCTION(
        BlueprintCallable,
        Category = "Chunk Generation"
    )
    void CancelAllChunks();


    // ========================================================================
    // STATE
    // ========================================================================

    UFUNCTION(
        BlueprintPure,
        Category = "Chunk Generation"
    )
    bool IsChunkGenerating(
        int32 ChunkX,
        int32 ChunkY
    ) const;


    UFUNCTION(
        BlueprintPure,
        Category = "Chunk Generation"
    )
    int32 GetActiveChunkCount() const;


public:

    // ========================================================================
    // CHUNK SETTINGS
    // ========================================================================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Chunk Generation"
    )
    int32 Resolution = 64;


    /*
     * Resolution divisor provides a simple LOD mechanism.
     *
     * Example:
     *
     *     Resolution = 64
     *     Divisor = 1 -> 64
     *     Divisor = 2 -> 32
     *     Divisor = 4 -> 16
     *     Divisor = 8 -> 8
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Chunk Generation",
        meta = (
            ClampMin = "1",
            UIMin = "1"
            )
    )
    int32 ResolutionDivisor = 1;


    /*
     * Physical dimensions of one chunk.
     *
     * The resolution changes sampling density, but this physical
     * size remains unchanged.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Chunk Generation",
        meta = (
            ClampMin = "1.0",
            UIMin = "1.0"
            )
    )
    float ChunkSize = 1000.0f;


    // ========================================================================
    // TERRAIN
    // ========================================================================

    /*
     * Global deterministic terrain seed.
     *
     * This is passed to FNoise_Generator once during BeginPlay.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Terrain"
    )
    int32 NoiseSeed = 123456;


public:

    // ========================================================================
    // EVENTS
    // ========================================================================

    UPROPERTY(
        BlueprintAssignable,
        Category = "Chunk Generation"
    )
    FOnChunkGenerated OnChunkGenerated;


private:

    // ========================================================================
    // ACTIVE GENERATORS
    // ========================================================================

    TMap<
        FIntPoint,
        TSharedPtr<Chunk_Generator>
    > ActiveGenerators;
};
