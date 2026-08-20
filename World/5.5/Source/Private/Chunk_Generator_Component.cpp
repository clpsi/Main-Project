#include "Chunk_Generator_Component.h"

#include "Chunk_Generator.h"


UChunk_Generator_Component::UChunk_Generator_Component()
{
    PrimaryComponentTick.bCanEverTick = false;
}


// ============================================================================
// END PLAY
// ============================================================================

void UChunk_Generator_Component::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
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
    int32 ChunkY)
{
    const FIntPoint ChunkCoordinate(
        ChunkX,
        ChunkY
    );


    /*
     * Don't generate the same chunk twice.
     */
    if (ActiveGenerators.Contains(
        ChunkCoordinate))
    {
        return;
    }


    /*
     * Convert chunk coordinates into world space.
     *
     * This can later be adjusted if your terrain uses another
     * coordinate convention.
     */
    const FVector ChunkLocation(
        static_cast<float>(ChunkX) * ChunkSize,
        static_cast<float>(ChunkY) * ChunkSize,
        0.0f
    );


    TSharedPtr<Chunk_Generator> Generator =
        MakeShared<Chunk_Generator>(
            Resolution,
            ChunkSize,
            ChunkLocation
        );


    ActiveGenerators.Add(
        ChunkCoordinate,
        Generator
    );


    /*
     * Weak reference to this component.
     *
     * If the component disappears while generation is running,
     * the completion callback simply does nothing.
     */
    TWeakObjectPtr<UChunk_Generator_Component>
        WeakThis(this);


    Generator->GenerateAsync(
        [
            WeakThis,
                ChunkCoordinate,
                Generator
        ](bool bWasCancelled)
        {
            /*
             * This callback is guaranteed to run on the
             * Game Thread by Chunk_Generator.
             */
            UChunk_Generator_Component* Component =
                WeakThis.Get();


            if (!Component)
            {
                return;
            }


            /*
             * If cancellation occurred, don't send incomplete
             * data to Blueprint.
             */
            if (bWasCancelled)
            {
                /*
                 * The chunk may already have been removed by
                 * CancelChunk(), so Remove() is harmless.
                 */
                Component->ActiveGenerators.Remove(
                    ChunkCoordinate
                );

                return;
            }


            /*
             * Copy the generated data.
             *
             * We are on the Game Thread now.
             */
            const TArray<FVector> Vertices =
                Generator->Vertices;


            const TArray<int32> Triangles =
                Generator->Triangles;


            const TArray<FVector> Normals =
                Generator->Normals;


            const TArray<FVector2D> UV0 =
                Generator->UV0;


            const TArray<FVector> CollisionVertices =
                Generator->CollisionVertices;


            const TArray<int32> CollisionTriangles =
                Generator->CollisionTriangles;


            /*
             * Generation is no longer active.
             */
            Component->ActiveGenerators.Remove(
                ChunkCoordinate
            );


            /*
             * Notify Blueprint.
             */
            Component->OnChunkGenerated.Broadcast(
                ChunkCoordinate.X,
                ChunkCoordinate.Y,

                Vertices,
                Triangles,
                Normals,
                UV0,

                CollisionVertices,
                CollisionTriangles
            );
        }
            );
}


// ============================================================================
// CANCEL ONE CHUNK
// ============================================================================

void UChunk_Generator_Component::CancelChunk(
    int32 ChunkX,
    int32 ChunkY)
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


    /*
     * Request cancellation.
     */
    Generator->Cancel();


    /*
     * Remove it from the active set immediately.
     *
     * The generator itself is still alive because the asynchronous
     * task owns a shared reference to it.
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
    /*
     * Request cancellation for every active generator.
     */
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
     * We don't need to keep them in the active map anymore.
     *
     * Their worker tasks still hold shared references until they
     * have completely stopped.
     */
    ActiveGenerators.Empty();
}


// ============================================================================
// STATE
// ============================================================================

bool UChunk_Generator_Component::IsChunkGenerating(
    int32 ChunkX,
    int32 ChunkY) const
{
    return ActiveGenerators.Contains(
        FIntPoint(
            ChunkX,
            ChunkY
        )
    );
}


int32 UChunk_Generator_Component::GetActiveChunkCount() const
{
    return ActiveGenerators.Num();
}
