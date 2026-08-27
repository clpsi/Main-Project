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


    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        if (ShouldStop())
        {
            return;
        }


        const float WorldY =
            ChunkWorldY +
            static_cast<float>(Y) * Step;


        const float V =
            static_cast<float>(Y)
            * InverseResolution;


        for (int32 X = 0; X <= Resolution; ++X)
        {
            if (ShouldStop())
            {
                return;
            }


            const float WorldX =
                ChunkWorldX +
                static_cast<float>(X) * Step;


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

        }
    }

    // ========================================================================
    // SKIRT
    // ========================================================================
    //
    // Only generate skirts for lower-resolution chunks.
    //
    // ========================================================================

    if (Resolution >= 64)
    {
        return;
    }


    const float SkirtDepth =
        TerrainConfig::HeightScale;


    const int32 SkirtVerticesPerEdge =
        VerticesPerSide;


    const int32 SkirtStartIndex =
        Vertices.Num();


    const int32 SkirtVertexCount =
        SkirtVerticesPerEdge * 4;


    Vertices.AddUninitialized(
        SkirtVertexCount
    );


    UV0.AddUninitialized(
        SkirtVertexCount
    );


    // ========================================================================
    // NORTH
    // ========================================================================

    for (int32 X = 0; X <= Resolution; ++X)
    {
        const int32 TerrainIndex =
            Resolution * VerticesPerSide + X;


        const int32 SkirtIndex =
            SkirtStartIndex + X;


        Vertices[SkirtIndex] =
            Vertices[TerrainIndex];

        Vertices[SkirtIndex].Z -=
            SkirtDepth;


        UV0[SkirtIndex] =
            UV0[TerrainIndex];
    }


    // ========================================================================
    // SOUTH
    // ========================================================================

    const int32 SouthStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge;


    for (int32 X = 0; X <= Resolution; ++X)
    {
        const int32 TerrainIndex =
            X;


        const int32 SkirtIndex =
            SouthStart + X;


        Vertices[SkirtIndex] =
            Vertices[TerrainIndex];

        Vertices[SkirtIndex].Z -=
            SkirtDepth;


        UV0[SkirtIndex] =
            UV0[TerrainIndex];
    }


    // ========================================================================
    // WEST
    // ========================================================================

    const int32 WestStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge * 2;


    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        const int32 TerrainIndex =
            Y * VerticesPerSide;


        const int32 SkirtIndex =
            WestStart + Y;


        Vertices[SkirtIndex] =
            Vertices[TerrainIndex];

        Vertices[SkirtIndex].Z -=
            SkirtDepth;


        UV0[SkirtIndex] =
            UV0[TerrainIndex];
    }


    // ========================================================================
    // EAST
    // ========================================================================

    const int32 EastStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge * 3;


    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        const int32 TerrainIndex =
            Y * VerticesPerSide
            + Resolution;


        const int32 SkirtIndex =
            EastStart + Y;


        Vertices[SkirtIndex] =
            Vertices[TerrainIndex];

        Vertices[SkirtIndex].Z -=
            SkirtDepth;


        UV0[SkirtIndex] =
            UV0[TerrainIndex];
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

    // ========================================================================
    // NO SKIRT
    // ========================================================================

    if (Resolution >= 64)
    {
        return;
    }


    // ========================================================================
    // SKIRT INDEX INFORMATION
    // ========================================================================

    const int32 SkirtVerticesPerEdge =
        VerticesPerSide;


    const int32 SkirtStartIndex =
        VerticesPerSide *
        VerticesPerSide;

    const int32 TriangleCount =
        Resolution * 4 * 6;


    Triangles.AddUninitialized(
        TriangleCount
    );


    // ========================================================================
    // NORTH
    // ========================================================================

    const int32 NorthStart =
        SkirtStartIndex;


    for (int32 X = 0; X < Resolution; ++X)
    {
        const int32 TerrainLeft =
            Resolution * VerticesPerSide + X;


        const int32 TerrainRight =
            TerrainLeft + 1;


        const int32 SkirtLeft =
            NorthStart + X;


        const int32 SkirtRight =
            NorthStart + X + 1;


        Triangles[Index++] =
            TerrainLeft;

        Triangles[Index++] =
            SkirtLeft;

        Triangles[Index++] =
            SkirtRight;


        Triangles[Index++] =
            TerrainLeft;

        Triangles[Index++] =
            SkirtRight;

        Triangles[Index++] =
            TerrainRight;
    }


    // ========================================================================
    // SOUTH
    // ========================================================================

    const int32 SouthStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge;


    for (int32 X = 0; X < Resolution; ++X)
    {
        const int32 TerrainLeft =
            X;


        const int32 TerrainRight =
            X + 1;


        const int32 SkirtLeft =
            SouthStart + X;


        const int32 SkirtRight =
            SouthStart + X + 1;


        Triangles[Index++] =
            TerrainLeft;

        Triangles[Index++] =
            SkirtRight;

        Triangles[Index++] =
            SkirtLeft;


        Triangles[Index++] =
            TerrainLeft;

        Triangles[Index++] =
            TerrainRight;

        Triangles[Index++] =
            SkirtRight;
    }


    // ========================================================================
    // WEST
    // ========================================================================

    const int32 WestStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge * 2;


    for (int32 Y = 0; Y < Resolution; ++Y)
    {
        const int32 TerrainBottom =
            Y * VerticesPerSide;


        const int32 TerrainTop =
            TerrainBottom + VerticesPerSide;


        const int32 SkirtBottom =
            WestStart + Y;


        const int32 SkirtTop =
            WestStart + Y + 1;


        Triangles[Index++] =
            TerrainBottom;

        Triangles[Index++] =
            SkirtBottom;

        Triangles[Index++] =
            SkirtTop;


        Triangles[Index++] =
            TerrainBottom;

        Triangles[Index++] =
            SkirtTop;

        Triangles[Index++] =
            TerrainTop;
    }


    // ========================================================================
    // EAST
    // ========================================================================

    const int32 EastStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge * 3;


    for (int32 Y = 0; Y < Resolution; ++Y)
    {
        const int32 TerrainBottom =
            Y * VerticesPerSide
            + Resolution;


        const int32 TerrainTop =
            TerrainBottom + VerticesPerSide;


        const int32 SkirtBottom =
            EastStart + Y;


        const int32 SkirtTop =
            EastStart + Y + 1;


        Triangles[Index++] =
            TerrainBottom;

        Triangles[Index++] =
            SkirtTop;

        Triangles[Index++] =
            SkirtBottom;


        Triangles[Index++] =
            TerrainBottom;

        Triangles[Index++] =
            TerrainTop;

        Triangles[Index++] =
            SkirtTop;
    }
}


// ============================================================================
// NORMALS
// ============================================================================

void Chunk_Generator::GenerateNormals()
{
    if (Vertices.Num() == 0 || Resolution <= 0)
    {
        return;
    }

    const int32 VerticesPerSide =
        Resolution + 1;

    Normals.SetNumUninitialized(
        Vertices.Num()
    );

    const float Step =
        ChunkSize /
        static_cast<float>(Resolution);

    const float ChunkWorldX =
        static_cast<float>(ChunkCoordinate.X)
        * ChunkSize;

    const float ChunkWorldY =
        static_cast<float>(ChunkCoordinate.Y)
        * ChunkSize;

    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        if (ShouldStop())
        {
            return;
        }

        const float WorldY =
            ChunkWorldY +
            static_cast<float>(Y) * Step;

        for (int32 X = 0; X <= Resolution; ++X)
        {
            if (ShouldStop())
            {
                return;
            }

            const int32 Index =
                Y * VerticesPerSide + X;

            const float WorldX =
                ChunkWorldX +
                static_cast<float>(X) * Step;

            // ================================================================
            // SAMPLE GLOBAL TERRAIN
            // ================================================================

            const float HeightLeft =
                FNoise_Generator::GetHeight(
                    WorldX - Step,
                    WorldY
                );

            const float HeightRight =
                FNoise_Generator::GetHeight(
                    WorldX + Step,
                    WorldY
                );

            const float HeightDown =
                FNoise_Generator::GetHeight(
                    WorldX,
                    WorldY - Step
                );

            const float HeightUp =
                FNoise_Generator::GetHeight(
                    WorldX,
                    WorldY + Step
                );

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
                2.0f * Step
            );

            Normals[Index] =
                Normal.GetSafeNormal();
        }
    }

    // ========================================================================
    // NO SKIRT
    // ========================================================================

    if (Resolution >= 64)
    {
        return;
    }


    // ========================================================================
    // SKIRT NORMALS
    // ========================================================================

    const int32 SkirtVerticesPerEdge =
        VerticesPerSide;


    const int32 SkirtStartIndex = VerticesPerSide * VerticesPerSide;


    // ========================================================================
    // NORTH
    // ========================================================================

    for (int32 X = 0; X <= Resolution; ++X)
    {
        Normals[
            SkirtStartIndex + X
        ] =
            FVector(
                0.0f,
                1.0f,
                0.0f
            );
    }


    // ========================================================================
    // SOUTH
    // ========================================================================

    const int32 SouthStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge;


    for (int32 X = 0; X <= Resolution; ++X)
    {
        Normals[
            SouthStart + X
        ] =
            FVector(
                0.0f,
                -1.0f,
                0.0f
            );
    }


    // ========================================================================
    // WEST
    // ========================================================================

    const int32 WestStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge * 2;


    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        Normals[
            WestStart + Y
        ] =
            FVector(
                -1.0f,
                0.0f,
                0.0f
            );
    }


    // ========================================================================
    // EAST
    // ========================================================================

    const int32 EastStart =
        SkirtStartIndex
        + SkirtVerticesPerEdge * 3;


    for (int32 Y = 0; Y <= Resolution; ++Y)
    {
        Normals[
            EastStart + Y
        ] =
            FVector(
                1.0f,
                0.0f,
                0.0f
            );
    }
}
