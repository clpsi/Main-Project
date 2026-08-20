#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Chunk_Generator_Component.generated.h"


class Chunk_Generator;


// ============================================================================
// BLUEPRINT EVENT
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_EightParams(
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
    UV0,

    const TArray<FVector>&,
    CollisionVertices,

    const TArray<int32>&,
    CollisionTriangles
);


// ============================================================================
// COMPONENT
// ============================================================================

UCLASS(
    ClassGroup = (ProceduralTerrain),
    meta = (BlueprintSpawnableComponent)
)
class LANDSCAPE_PROJ_API UChunk_Generator_Component : public UActorComponent
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

    // ================================================================
    // GENERATION
    // ================================================================

    UFUNCTION(
        BlueprintCallable,
        Category = "Chunk Generation"
    )
    void GenerateChunk(
        int32 ChunkX,
        int32 ChunkY
    );


    // ================================================================
    // CANCELLATION
    // ================================================================

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


    // ================================================================
    // STATE
    // ================================================================

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

    // ================================================================
    // SETTINGS
    // ================================================================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Chunk Generation"
    )
    int32 Resolution = 64;


    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Chunk Generation"
    )
    float ChunkSize = 1000.0f;


    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Terrain Noise"
    )
    int32 NoiseSeed = 123456;


    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Terrain Noise"
    )
    float NoiseFrequency = 0.00008f;


    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Terrain Noise"
    )
    int32 NoiseOctaves = 6;


    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Terrain Noise"
    )
    float NoiseLacunarity = 2.0f;


    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Terrain Noise"
    )
    float NoisePersistence = 0.5f;


    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Terrain Noise"
    )
    float NoiseHeightScale = 500.0f;


public:

    // ================================================================
    // EVENTS
    // ================================================================

    UPROPERTY(
        BlueprintAssignable,
        Category = "Chunk Generation"
    )
    FOnChunkGenerated OnChunkGenerated;


private:

    // ================================================================
    // ACTIVE GENERATORS
    // ================================================================

    TMap<
        FIntPoint,
        TSharedPtr<Chunk_Generator>
    > ActiveGenerators;
};
