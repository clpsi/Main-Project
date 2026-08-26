#include "Chunk_Generator.h"

#include "Async/Async.h"
#include "Noise_Generator.h"


// ============================================================================
// CONSTRUCTOR
// ============================================================================

Chunk_Generator::Chunk_Generator(
    int32 InResolution,
    float InChunkSize,
    int32 InChunkX,
    int32 InChunkY
)
    : Resolution(InResolution)
    , ChunkSize(InChunkSize)
    , ChunkCoordinate(
        InChunkX,
        InChunkY
    )
{
}


// ============================================================================
// DESTRUCTOR
// ============================================================================

Chunk_Generator::~Chunk_Generator()
{
    Cancel();
}


// ============================================================================
// ASYNC GENERATION
// ============================================================================

void Chunk_Generator::GenerateAsync(
    FChunkGenerationComplete CompletionCallback
)
{
    bool Expected = false;

    /*
     * A generator represents exactly one generation job.
     *
     * Prevent the same generator from being started twice.
     */
    if (!bStarted.compare_exchange_strong(
        Expected,
        true,
        std::memory_order_acq_rel
    ))
    {
        return;
    }


    /*
     * Keep the generator alive until the worker and completion
     * callback have finished.
     */
    TSharedPtr<Chunk_Generator> Self =
        AsShared();


    Async(
        EAsyncExecution::ThreadPool,

        [
            Self,
                CompletionCallback = MoveTemp(CompletionCallback)
        ]() mutable
        {
            // ============================================================
            // VERTICES
            // ============================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateVertices();
            }


            // ============================================================
            // TRIANGLES
            // ============================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateTriangles();
            }


            // ============================================================
            // NORMALS
            // ============================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateNormals();
            }


            // ============================================================
            // FINISHED
            // ============================================================

            const bool bWasCancelled =
                Self->ShouldStop();


            Self->bFinished.store(
                true,
                std::memory_order_release
            );


            /*
             * Completion must happen on the Game Thread because
             * the receiving code may interact with UObjects,
             * ProceduralMeshComponent, Blueprint, etc.
             */
            AsyncTask(
                ENamedThreads::GameThread,

                [
                    Self,
                        CompletionCallback = MoveTemp(
                            CompletionCallback
                        ),
                        bWasCancelled
                ]() mutable
                {
                    if (CompletionCallback)
                    {
                        CompletionCallback(
                            bWasCancelled
                        );
                    }
                }
                    );
        }
            );
}


// ============================================================================
// CANCELLATION
// ============================================================================

void Chunk_Generator::Cancel()
{
    bCancelRequested.store(
        true,
        std::memory_order_release
    );
}


bool Chunk_Generator::IsCancelled() const
{
    return bCancelRequested.load(
        std::memory_order_acquire
    );
}


bool Chunk_Generator::IsFinished() const
{
    return bFinished.load(
        std::memory_order_acquire
    );
}


bool Chunk_Generator::ShouldStop() const
{
    return bCancelRequested.load(
        std::memory_order_relaxed
    );
}


// ============================================================================
// VERTICES + UVS
// ============================================================================

void Chunk_Generator::GenerateVertices()
{
    if (Resolution <= 0)
    {
        return;
    }


    // ========================================================================
    // GRID SIZE
    // ========================================================================

    const int32 VerticesPerSide =
        Resolution + 1;


    const int32 VertexCount =
        VerticesPerSide *
        VerticesPerSide;


    // ========================================================================
    // ALLOCATE EXACT SIZE
    // ========================================================================

    Vertices.SetNumUninitialized(
        VertexCount
    );


    UV0.SetNumUninitialized(
        VertexCount
    );


    // ========================================================================
    // GRID SPACING
    // ========================================================================

    const float Step =
        ChunkSize /
        static_cast<float>(Resolution);


    const float InverseResolution =
        1.0f /
        static_cast<float>(Resolution);


    // ========================================================================
    // CHUNK WORLD LOCATION
    // ========================================================================
    //
    // ChunkCoordinate identifies the chunk.
    //
    // ChunkSize determines its physical position.
    //
    // Example:
    //
    // ChunkCoordinate = (3, -2)
    //
    // World position =
    //
    //     X = 3 * ChunkSize
    //     Y = -2 * ChunkSize
    //
    // ========================================================================

    const float ChunkWorldX =
        static_cast<float>(
            ChunkCoordinate.X
            )
        * ChunkSize;


    const float ChunkWorldY =
        static_cast<float>(
            ChunkCoordinate.Y
            )
        * ChunkSize;


    // ========================================================================
    // GENERATE GRID
    // ========================================================================

    float WorldY =
        ChunkWorldY;


    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        if (ShouldStop())
        {
            return;
        }


        float WorldX =
            ChunkWorldX;


        const float V =
            static_cast<float>(Y)
            * InverseResolution;


        for (int32 X = 0; X <= Resolution; ++X)
        {
            if (ShouldStop())
            {
                return;
            }


            const int32 Index =
                Y * VerticesPerSide + X;


            const float U =
                static_cast<float>(X)
                * InverseResolution;


            // ================================================================
            // TERRAIN HEIGHT
            // ================================================================

            const float Height =
                FNoise_Generator::GetHeight(
                    WorldX,
                    WorldY
                );


            // ================================================================
            // VERTEX
            // ================================================================

            Vertices[Index] =
                FVector(
                    WorldX,
                    WorldY,
                    Height
                );


            // ================================================================
            // UV
            // ================================================================

            UV0[Index] =
                FVector2D(
                    U,
                    V
                );


            // ================================================================
            // NEXT X
            // ================================================================

            WorldX += Step;
        }


        // ====================================================================
        // NEXT Y
        // ====================================================================

        WorldY += Step;
    }
}


// ============================================================================
// TRIANGLES
// ============================================================================

void Chunk_Generator::GenerateTriangles()
{
    if (Resolution <= 0)
    {
        return;
    }


    // ========================================================================
    // GRID SIZE
    // ========================================================================

    const int32 VerticesPerSide =
        Resolution + 1;


    // ========================================================================
    // EXACT TRIANGLE INDEX COUNT
    // ========================================================================

    const int32 TriangleIndexCount =
        Resolution *
        Resolution *
        6;


    Triangles.SetNumUninitialized(
        TriangleIndexCount
    );


    // ========================================================================
    // WRITE INDEX BUFFER DIRECTLY
    // ========================================================================

    int32 Index =
        0;


    for (int32 Y = 0; Y < Resolution; ++Y)
    {
        if (ShouldStop())
        {
            return;
        }


        for (int32 X = 0; X < Resolution; ++X)
        {
            if (ShouldStop())
            {
                return;
            }


            const int32 BottomLeft =
                Y * VerticesPerSide + X;


            const int32 BottomRight =
                BottomLeft + 1;


            const int32 TopLeft =
                BottomLeft + VerticesPerSide;


            const int32 TopRight =
                TopLeft + 1;


            // =================================================================
            // TRIANGLE 1
            // =================================================================

            Triangles[Index++] =
                BottomLeft;

            Triangles[Index++] =
                TopLeft;

            Triangles[Index++] =
                TopRight;


            // =================================================================
            // TRIANGLE 2
            // =================================================================

            Triangles[Index++] =
                BottomLeft;

            Triangles[Index++] =
                TopRight;

            Triangles[Index++] =
                BottomRight;
        }
    }
}


// ============================================================================
// NORMALS
// ============================================================================

void Chunk_Generator::GenerateNormals()
{
    if (Vertices.Num() == 0)
    {
        return;
    }


    const int32 VerticesPerSide =
        Resolution + 1;


    Normals.SetNumUninitialized(
        Vertices.Num()
    );


    // ========================================================================
    // GRID SPACING
    // ========================================================================

    const float Step =
        ChunkSize /
        static_cast<float>(Resolution);


    /*
     * This is the Z component of the tangent-derived normal.
     *
     * For:
     *
     *     N = (-dH/dX, -dH/dY, 1)
     *
     * using central differences gives us:
     *
     *     (-dX, -dY, 2 * Step)
     */
    const float NormalZ =
        2.0f * Step;


    // ========================================================================
    // CALCULATE NORMALS
    // ========================================================================

    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        if (ShouldStop())
        {
            return;
        }


        for (int32 X = 0; X <= Resolution; ++X)
        {
            if (ShouldStop())
            {
                return;
            }


            const int32 Index =
                Y * VerticesPerSide + X;


            // ================================================================
            // NEIGHBOUR COORDINATES
            // ================================================================

            const int32 LeftX =
                FMath::Max(
                    X - 1,
                    0
                );


            const int32 RightX =
                FMath::Min(
                    X + 1,
                    Resolution
                );


            const int32 DownY =
                FMath::Max(
                    Y - 1,
                    0
                );


            const int32 UpY =
                FMath::Min(
                    Y + 1,
                    Resolution
                );


            // ================================================================
            // NEIGHBOUR HEIGHTS
            // ================================================================

            const float HeightLeft =
                Vertices[
                    Y * VerticesPerSide + LeftX
                ].Z;


            const float HeightRight =
                Vertices[
                    Y * VerticesPerSide + RightX
                ].Z;


            const float HeightDown =
                Vertices[
                    DownY * VerticesPerSide + X
                ].Z;


            const float HeightUp =
                Vertices[
                    UpY * VerticesPerSide + X
                ].Z;


            // ================================================================
            // CENTRAL DIFFERENCES
            // ================================================================

            const float DX =
                HeightRight -
                HeightLeft;


            const float DY =
                HeightUp -
                HeightDown;


            // ================================================================
            // NORMAL
            // ================================================================

            FVector Normal(
                -DX,
                -DY,
                NormalZ
            );


            Normals[Index] =
                Normal.GetSafeNormal();
        }
    }
}
