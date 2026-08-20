#include "Chunk_Generator.h"

#include "Async/Async.h"


Chunk_Generator::Chunk_Generator(
    int32 InResolution,
    float InChunkSize,
    const FVector& InChunkLocation)
    : Resolution(InResolution)
    , ChunkSize(InChunkSize)
    , ChunkLocation(InChunkLocation)
{
}


Chunk_Generator::~Chunk_Generator()
{
    Cancel();
}


// ============================================================================
// ASYNC GENERATION
// ============================================================================

void Chunk_Generator::GenerateAsync(
    FChunkGenerationComplete CompletionCallback)
{
    bool Expected = false;


    /*
     * A generator represents exactly one generation job.
     *
     * Prevent accidentally starting the same generator twice.
     */
    if (!bStarted.compare_exchange_strong(
        Expected,
        true,
        std::memory_order_acq_rel))
    {
        return;
    }


    /*
     * Keep the generator alive until the worker and completion
     * callback are completely finished with it.
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
            // ========================================================
            // GENERATE VERTICES
            // ========================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateVertices();
            }


            // ========================================================
            // GENERATE TRIANGLES
            // ========================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateTriangles();
            }


            // ========================================================
            // GENERATE NORMALS
            // ========================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateNormals();
            }


            // ========================================================
            // GENERATE UVS
            // ========================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateUVs();
            }


            // ========================================================
            // GENERATE COLLISION
            // ========================================================

            if (!Self->ShouldStop())
            {
                Self->GenerateCollision();
            }


            // ========================================================
            // FINISHED
            // ========================================================

            const bool bWasCancelled =
                Self->ShouldStop();


            Self->bFinished.store(
                true,
                std::memory_order_release
            );


            /*
             * We are currently on the worker thread.
             *
             * The callback must therefore be moved onto the
             * Game Thread before anything UObject/Blueprint-related
             * happens.
             */
            AsyncTask(
                ENamedThreads::GameThread,
                [
                    Self,
                        CompletionCallback = MoveTemp(CompletionCallback),
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
// VERTICES
// ============================================================================

void Chunk_Generator::GenerateVertices()
{
    if (Resolution <= 0)
    {
        return;
    }


    const int32 VerticesPerSide =
        Resolution + 1;


    const int32 VertexCount =
        VerticesPerSide * VerticesPerSide;


    Vertices.Reserve(
        VertexCount
    );


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


            const float AlphaX =
                static_cast<float>(X) /
                static_cast<float>(Resolution);


            const float AlphaY =
                static_cast<float>(Y) /
                static_cast<float>(Resolution);


            const float LocalX =
                AlphaX * ChunkSize;


            const float LocalY =
                AlphaY * ChunkSize;


            /*
             * Terrain height will eventually come from
             * your separate noise system.
             */
            const float Height = 0.0f;


            Vertices.Add(
                ChunkLocation +
                FVector(
                    LocalX,
                    LocalY,
                    Height
                )
            );
        }
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


    const int32 VerticesPerSide =
        Resolution + 1;


    const int32 QuadCount =
        Resolution * Resolution;


    Triangles.Reserve(
        QuadCount * 6
    );


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


            /*
             * Triangle 1
             */
            Triangles.Add(BottomLeft);
            Triangles.Add(TopLeft);
            Triangles.Add(TopRight);


            /*
             * Triangle 2
             */
            Triangles.Add(BottomLeft);
            Triangles.Add(TopRight);
            Triangles.Add(BottomRight);
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


    Normals.SetNumZeroed(
        Vertices.Num()
    );


    for (
        int32 TriangleIndex = 0;
        TriangleIndex < Triangles.Num();
        TriangleIndex += 3
        )
    {
        if (ShouldStop())
        {
            return;
        }


        const int32 IndexA =
            Triangles[TriangleIndex];


        const int32 IndexB =
            Triangles[TriangleIndex + 1];


        const int32 IndexC =
            Triangles[TriangleIndex + 2];


        const FVector& A =
            Vertices[IndexA];


        const FVector& B =
            Vertices[IndexB];


        const FVector& C =
            Vertices[IndexC];


        const FVector EdgeA =
            B - A;


        const FVector EdgeB =
            C - A;


        const FVector Normal =
            FVector::CrossProduct(
                EdgeB,
                EdgeA
            );


        Normals[IndexA] += Normal;
        Normals[IndexB] += Normal;
        Normals[IndexC] += Normal;
    }


    for (FVector& Normal : Normals)
    {
        if (ShouldStop())
        {
            return;
        }


        Normal.Normalize();
    }
}


// ============================================================================
// UVS
// ============================================================================

void Chunk_Generator::GenerateUVs()
{
    if (Resolution <= 0)
    {
        return;
    }


    UV0.Reserve(
        Vertices.Num()
    );


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


            const float U =
                static_cast<float>(X) /
                static_cast<float>(Resolution);


            const float V =
                static_cast<float>(Y) /
                static_cast<float>(Resolution);


            UV0.Add(
                FVector2D(
                    U,
                    V
                )
            );
        }
    }
}


// ============================================================================
// COLLISION
// ============================================================================

void Chunk_Generator::GenerateCollision()
{
    if (ShouldStop())
    {
        return;
    }


    /*
     * Temporary implementation:
     * collision uses the same mesh as the render mesh.
     *
     * Later this can become a lower-resolution mesh.
     */
    CollisionVertices =
        Vertices;


    if (ShouldStop())
    {
        return;
    }


    CollisionTriangles =
        Triangles;
}
