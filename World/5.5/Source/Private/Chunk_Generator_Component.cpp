#include "Chunk_Generator_Component.h"

#include "Chunk_Generator.h"
#include "Noise_Generator.h"


// ============================================================================
// CONSTRUCTOR
// ============================================================================

UChunk_Generator_Component::UChunk_Generator_Component()
{
    PrimaryComponentTick.bCanEverTick = false;
}


// ============================================================================
// BEGIN PLAY
// ============================================================================

void UChunk_Generator_Component::BeginPlay()
{
    Super::BeginPlay();


    /*
     * Initialize the global terrain seed.
     *
     * This should remain unchanged after initialization.
     */
    FNoise_Generator::Initialize(
        NoiseSeed
    );

    // ========================================================================
    // DEBUG: TEST WORLD REPEAT
    // ========================================================================

    /*const float Repeat = TerrainConfig::WorldRepeatSize;

    // Arbitrary test coordinate
    const float X = 123.45f;
    const float Y = 678.90f;

    // --- Explicit origin tests ---

    const float Height_00 =
        FNoise_Generator::GetHeight(
            0.0f,
            0.0f
        );

    const float Height_RepeatX =
        FNoise_Generator::GetHeight(
            Repeat,
            0.0f
        );

    const float Height_RepeatY =
        FNoise_Generator::GetHeight(
            0.0f,
            Repeat
        );


    // --- Arbitrary X/Y tests ---

    const float Height_XY =
        FNoise_Generator::GetHeight(
            X,
            Y
        );

    const float Height_XRepeatY =
        FNoise_Generator::GetHeight(
            X + Repeat,
            Y
        );

    const float Height_XYRepeat =
        FNoise_Generator::GetHeight(
            X,
            Y + Repeat
        );

    const float Height_XRepeatYRepeat =
        FNoise_Generator::GetHeight(
            X + Repeat,
            Y + Repeat
        );


    // ========================================================================
    // LOG RESULTS
    // ========================================================================

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("========== NOISE REPEAT DEBUG ==========")
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("WorldRepeatSize: %f"),
        Repeat
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("GetHeight(0, 0)                         = %f"),
        Height_00
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("GetHeight(Repeat, 0)                   = %f"),
        Height_RepeatX
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("GetHeight(0, Repeat)                   = %f"),
        Height_RepeatY
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("GetHeight(%f, %f)                     = %f"),
        X,
        Y,
        Height_XY
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("GetHeight(%f + Repeat, %f)             = %f"),
        X,
        Y,
        Height_XRepeatY
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("GetHeight(%f, %f + Repeat)             = %f"),
        X,
        Y,
        Height_XYRepeat
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("GetHeight(%f + Repeat, %f + Repeat)    = %f"),
        X,
        Y,
        Height_XRepeatYRepeat
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("=========================================")
    );*/
}


// ============================================================================
// END PLAY
// ============================================================================

void UChunk_Generator_Component::EndPlay(
    const EEndPlayReason::Type EndPlayReason
)
{
    CancelAllChunks();


    Super::EndPlay(
        EndPlayReason
    );
}


// ============================================================================
// GENERATE CHUNK
// ============================================================================

void UChunk_Generator_Component::GenerateChunk(
    int32 ChunkX,
    int32 ChunkY,
    int32 ResolutionDivisor
)
{
    const FIntPoint ChunkCoordinate(
        ChunkX,
        ChunkY
    );


    // ========================================================================
    // DUPLICATE CHECK
    // ========================================================================

    if (ActiveGenerators.Contains(
        ChunkCoordinate
    ))
    {
        return;
    }


    // ========================================================================
    // RESOLUTION
    // ========================================================================

    ResolutionDivisor = FMath::RoundUpToPowerOfTwo(FMath::Clamp(ResolutionDivisor, 1, 16));

    const int32 SafeDivisor =
        FMath::Max(
            1,
            ResolutionDivisor
        );


    const int32 ActualResolution =
        FMath::Max(
            1,
            Resolution / SafeDivisor
        );


    // ========================================================================
    // GENERATOR
    // ========================================================================

    TSharedPtr<Chunk_Generator> Generator =
        MakeShared<Chunk_Generator>(
            ActualResolution,
            ChunkSize,

            ChunkX,
            ChunkY
        );


    ActiveGenerators.Add(
        ChunkCoordinate,
        Generator
    );


    // ========================================================================
    // WEAK COMPONENT REFERENCE
    // ========================================================================

    TWeakObjectPtr<UChunk_Generator_Component>
        WeakThis(this);


    // ========================================================================
    // GENERATE
    // ========================================================================

    Generator->GenerateAsync(
        [
            WeakThis,
                ChunkCoordinate,
                Generator
        ](
            bool bWasCancelled
            )
        {
            // =================================================================
            // COMPONENT VALIDITY
            // =================================================================

            UChunk_Generator_Component* Component =
                WeakThis.Get();


            if (!Component)
            {
                return;
            }


            // =================================================================
            // CANCELLATION
            // =================================================================

            if (bWasCancelled)
            {
                /*
                 * The chunk may already have been removed by CancelChunk().
                 *
                 * Remove() is therefore intentionally harmless here.
                 */
                Component->ActiveGenerators.Remove(
                    ChunkCoordinate
                );


                return;
            }


            // =================================================================
            // COPY GENERATED DATA
            // =================================================================

            const TArray<FVector> Vertices =
                Generator->Vertices;


            const TArray<int32> Triangles =
                Generator->Triangles;


            const TArray<FVector> Normals =
                Generator->Normals;


            const TArray<FVector2D> UV0 =
                Generator->UV0;



            // =================================================================
            // REMOVE FROM ACTIVE
            // =================================================================

            Component->ActiveGenerators.Remove(
                ChunkCoordinate
            );


            // =================================================================
            // BLUEPRINT EVENT
            // =================================================================

            Component->OnChunkGenerated.Broadcast(
                ChunkCoordinate.X,
                ChunkCoordinate.Y,

                Vertices,
                Triangles,
                Normals,
                UV0
            );
        }
    );
}


// ============================================================================
// CANCEL ONE CHUNK
// ============================================================================

void UChunk_Generator_Component::CancelChunk(
    int32 ChunkX,
    int32 ChunkY
)
{
    const FIntPoint ChunkCoordinate(
        ChunkX,
        ChunkY
    );


    TSharedPtr<Chunk_Generator>* GeneratorPtr =
        ActiveGenerators.Find(
            ChunkCoordinate
        );


    if (!GeneratorPtr)
    {
        return;
    }


    TSharedPtr<Chunk_Generator> Generator =
        *GeneratorPtr;


    if (Generator.IsValid())
    {
        Generator->Cancel();
    }


    /*
     * Remove immediately from the active map.
     *
     * The asynchronous task still owns a shared reference.
     */
    ActiveGenerators.Remove(
        ChunkCoordinate
    );
}


// ============================================================================
// CANCEL ALL
// ============================================================================

void UChunk_Generator_Component::CancelAllChunks()
{
    for (
        const TPair<
        FIntPoint,
        TSharedPtr<Chunk_Generator>
        >& Pair
        : ActiveGenerators
        )
    {
        if (Pair.Value.IsValid())
        {
            Pair.Value->Cancel();
        }
    }


    /*
     * Worker tasks retain their own shared references until finished.
     */
    ActiveGenerators.Empty();
}


// ============================================================================
// STATE
// ============================================================================

bool UChunk_Generator_Component::IsChunkGenerating(
    int32 ChunkX,
    int32 ChunkY
) const
{
    return ActiveGenerators.Contains(
        FIntPoint(
            ChunkX,
            ChunkY
        )
    );
}


// ============================================================================

int32 UChunk_Generator_Component::GetActiveChunkCount() const
{
    return ActiveGenerators.Num();
}
