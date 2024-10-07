// Fill out your copyright notice in the Description page of Project Settings.

#include "cppFunctions.h"

#include "Components/DynamicMeshComponent.h"
#include "Components/StaticMeshComponent.h"

#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicAttribute.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"
#include "MeshBoundaryLoops.h"

#include "Polygroups/PolygroupSet.h"
#include "Polygroups/PolygroupsGenerator.h"

#include "GeometryScript/MeshPolygroupFunctions.h"
#include "GeometryScript/GeometryScriptTypes.h"

#include "Engine/StaticMesh.h"

#include "GameFramework/Actor.h"
#include "Engine/GameEngine.h"
#include "Engine/World.h"

#include <DocObj.h>
#include "IndexTypes.h"

#include "Math/Quat.h"
#include "Math/Vector.h"
#include "Containers/Array.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
//D:\Programs\UE_5.4\Engine\Plugins\Runtime\GeometryScripting\Source\GeometryScriptingCore\Private

using namespace UE::Geometry;


bool UcppFunctions::CopyPolygroupToMesh(UDynamicMeshComponent* InputMesh, UDynamicMeshComponent* TargetMesh, int32 Polygroup)
{
    if (InputMesh && InputMesh->GetMesh() && TargetMesh && TargetMesh->GetMesh())
    {
        const UE::Geometry::FDynamicMesh3* ReadMesh = InputMesh->GetMesh();

        FPolygroupLayer InputGroupLayer;
        InputGroupLayer.bIsDefaultLayer = true;
        if (InputGroupLayer.CheckExists(ReadMesh) == false)
        {
            //print error
            return false;
        }

        FPolygroupSet Groups(ReadMesh, InputGroupLayer);

        FDynamicMesh3* Mesh = InputMesh->GetMesh();
        FDynamicMesh3* TMesh = TargetMesh->GetMesh();
        TMap<int32, int32> VertexMapping;  // Mapping from source vertex ID to target vertex ID
        TArray<int32> PolygroupVertices;  // Store the vertices that belong to the polygroup

        for (int32 TriangleID = 0; TriangleID < ReadMesh->MaxTriangleID(); ++TriangleID)
        {
            if (ReadMesh->IsTriangle(TriangleID))
            {
                if (Groups.GetGroup(TriangleID) == Polygroup)
                {
                    UE::Geometry::FIndex3i Vertices = Mesh->GetTriangle(TriangleID);
                    for (int i = 0; i < 3; i++)
                    {
                        FVector3d Position = Mesh->GetVertex(Vertices[i]);
                        FVector3f Normal = Mesh->GetVertexNormal(Vertices[i]);
                        FVector2f UV = Mesh->GetVertexUV(Vertices[i]);

                        // Add the vertex to the new mesh
                        int32 NewVertexID = TMesh->AppendVertex(Position);
                        TMesh->SetVertexNormal(NewVertexID, Normal);
                        TMesh->SetVertexUV(NewVertexID, UV);

                        // Map the original vertex ID to the new vertex ID
                        VertexMapping.Add(Vertices[i], NewVertexID);

                        PolygroupVertices.Add(VertexMapping[Vertices[i]]);
                    }
                }
            }
        }
        
        for (int32 i = 0; i < PolygroupVertices.Num(); i += 3)
        {
            int32 V0 = PolygroupVertices[i];
            int32 V1 = PolygroupVertices[i + 1];
            int32 V2 = PolygroupVertices[i + 2];

            // Add the triangle to the new mesh
            int32 NewTriangleID = TMesh->AppendTriangle(V0, V1, V2);
        }

        // Notify that the mesh has been updated
        TargetMesh->NotifyMeshUpdated();
        int32 count = TMesh->VertexCount();
        int32 triangle = TMesh->TriangleCount();
        TargetMesh->MarkRenderStateDirty();
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Vertices Count: %d"), count));
        UE_LOG(LogBlueprintUserMessages, Log, TEXT("Vertices Count: %d"), count);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Triangle Count: %d"), triangle));
        UE_LOG(LogBlueprintUserMessages, Log, TEXT("Triangle Count: %d"), triangle);
        return true;
    }
    return false;
}

bool UcppFunctions::SweepComponent(UPrimitiveComponent* ComponentToSweep, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, TArray<FHitResult>& OutHits)
{
    UWorld* CurrentWorld = ComponentToSweep->GetOwner()->GetWorld();
    if (!CurrentWorld) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Didn't find World"));
        return false;
    }

    FComponentQueryParams ComponentQueryParams;

    //ComponentQueryParams.AddIgnoredActor(ActorToIgnore);
    bool bHit = CurrentWorld->ComponentSweepMultiByChannel(
        OutHits,
        ComponentToSweep,
        Start,
        End,
        Rot,
        TraceChannel,
        ComponentQueryParams
    );

    return bHit;
}

bool UcppFunctions::TraceFromInsideMesh(AActor* ActorToIgnore, bool ShouldIgnore, const FVector& Start, const FVector& End, FHitResult& OutHit)
{
    // Set up the collision query parameters
    FCollisionQueryParams TraceParams(FName(TEXT("EdgeTrace")), true);
    TraceParams.bReturnPhysicalMaterial = false;
    TraceParams.bFindInitialOverlaps = false; // Ignore the initial overlap
    if (ShouldIgnore) 
    {
        TraceParams.AddIgnoredActor(ActorToIgnore);
    } 

    // Perform the line trace
    bool bHit = ActorToIgnore->GetWorld()->LineTraceSingleByChannel(
        OutHit,
        Start,
        End,
        ECC_Visibility, // Use the appropriate collision channel
        TraceParams
    );

    // Draw debug line to visualize the trace (optional)
        FColor LineColor = bHit ? FColor::Red : FColor::Green;
        DrawDebugLine(ActorToIgnore->GetWorld(), Start, End, FColor::Yellow, false, 2.0f, 0, 2.0f);
        if (bHit)
        {
            DrawDebugPoint(ActorToIgnore->GetWorld(), OutHit.ImpactPoint, 10.0f, FColor::Blue, false, 2.0f);
        }

    return bHit;
}

void UcppFunctions::EnableDoubleSidedGeometry(UDynamicMeshComponent* InputMesh)
{
    if (InputMesh)
    {
        FDynamicMesh3* Mesh = InputMesh->GetMesh();
        if (Mesh)
        {
            // Duplicate each triangle and reverse the vertex order to create a double-sided effect
            for (int32 TID = 0; TID < Mesh->MaxTriangleID(); ++TID)
            {
                if (Mesh->IsTriangle(TID))
                {
                    FIndex3i Triangle = Mesh->GetTriangle(TID);

                    // Add a new triangle with reversed vertex order
                    Mesh->AppendTriangle(Triangle.C, Triangle.B, Triangle.A);
                }
            }

            // Update the mesh to include the new triangles
            InputMesh->NotifyMeshUpdated();

            //Has No collision
            //DynamicMeshComponent->UpdateCollision();
        }
    }
}

void UcppFunctions::FillHolesInDynamicMeshComponent(UDynamicMeshComponent* MeshComponent)
{
    if (!MeshComponent)
    {
        return;
    }

    FDynamicMesh3* Mesh = MeshComponent->GetMesh();

    // Ensure the mesh has valid triangles
    if (Mesh->MaxTriangleID() == 0)
    {
        return;
    }

    // Create an editor for the dynamic mesh
    FDynamicMeshEditor Editor(Mesh);
    FMeshBoundaryLoops BoundaryLoops(Mesh, true);

    if (BoundaryLoops.Compute())
    {
        for (const FEdgeLoop& Loop : BoundaryLoops.Loops)
        {
            // Calculate the centroid of the boundary loop
            FVector3d Centroid(0, 0, 0);
            for (int32 VertexID : Loop.Vertices)
            {
                Centroid += Mesh->GetVertex(VertexID);
            }
            Centroid /= (double)Loop.Vertices.Num();
            int32 NewVertexID = Mesh->AppendVertex(Centroid);
            // Prepare a mesh edit result to capture the output
            FDynamicMeshEditResult EditResult;

            // Fill the hole using AddTriangleFan_OrderedVertexLoop
            Editor.AddTriangleFan_OrderedVertexLoop(NewVertexID, Loop.Vertices, FDynamicMesh3::InvalidID, EditResult);
        }
    }


    // Update the mesh after modification
    MeshComponent->NotifyMeshUpdated();
}

void UcppFunctions::MoveVertices(UDynamicMeshComponent* InputMesh, int32 Polygroup, float Amount, int32 xAmount, int32 yAmount)
{
    if (InputMesh)
    {
        FPolygroupLayer InputGroupLayer;
        InputGroupLayer.bIsDefaultLayer = true;
        FDynamicMesh3* Mesh = InputMesh->GetMesh();

        if (InputGroupLayer.CheckExists(Mesh) == false)
        {
            UE_LOG(LogBlueprintUserMessages, Log, TEXT("Failed"));
        }

        TArray<FVector3d> TriangleNormals;

        FPolygroupSet Groups(Mesh, InputGroupLayer);
        TArray<int32> VertexIndices;

        for (int32 TriangleID = 0; TriangleID < Mesh->MaxTriangleID(); ++TriangleID)
        {
            if (Mesh->IsTriangle(TriangleID))
            {
                FVector3d Normal = Mesh->GetTriNormal(TriangleID);
                TriangleNormals.Add(Normal);

                if (Groups.GetGroup(TriangleID) == Polygroup)
                {
                    FIndex3i UniqueVertices = Mesh->GetTriangle(TriangleID);
                    VertexIndices.AddUnique(UniqueVertices.A);
                    VertexIndices.AddUnique(UniqueVertices.B);
                    VertexIndices.AddUnique(UniqueVertices.C);
                }
            }
        }

        FVector3d NormalSum(0, 0, 0);

        for (const FVector3d& Normal : TriangleNormals)
        {
            NormalSum += Normal;
        }

        FVector3d AverageNormal(0, 0, 0);

        if (TriangleNormals.Num() > 0)
        {
            AverageNormal = NormalSum / static_cast<double>(TriangleNormals.Num());
        }

        AverageNormal.Normalize();
        FVector3d NormalizedVector = AverageNormal.GetSafeNormal(); //Plane Normal
        // Choose an arbitrary vector that is not parallel to NormalVector
        FVector3d ArbitraryVector;
        if (FMath::Abs(NormalizedVector.X) < 0.99)
        {
            ArbitraryVector = FVector3d(1, 0, 0); // X-axis
        }
        else
        {
            ArbitraryVector = FVector3d(0, 1, 0); // Y-axis
        }

        // Compute the first orthogonal vector using the cross product
        FVector3d VX = FVector3d::CrossProduct(NormalizedVector, ArbitraryVector);
        VX.Normalize();

        // Compute the second orthogonal vector using the cross product again
        FVector3d VY = FVector3d::CrossProduct(NormalizedVector, VX);
        VY.Normalize();

        TArray<FVector2D> ProjectedVertices;
        FVector2D maxX = FVector2D(-100000.0f, -100000.0f);
        int32 maxIndex = -1;
        FVector2D minX = FVector2D(100000.0f, 100000.0f);
        int32 minIndex = -1;
        FVector2D maxY = FVector2D(-100000.0f, -100000.0f);
        int32 mayIndex = -1;
        FVector2D minY = FVector2D(100000.0f, 100000.0f);
        int32 miyIndex = -1;
        
        for (int32 VertexIndex : VertexIndices)
        {
            FVector3d VertexPosition = Mesh->GetVertex(VertexIndex);

            // Calculate the distance from the vertex to the plane along the normal
            float Distance = FVector3d::DotProduct(VertexPosition, NormalizedVector);

            // Calculate the projection of the vertex onto the plane
            FVector3d ProjectedVertex = VertexPosition - (Distance * NormalizedVector);

            // Calculate the local 2D coordinates in the plane's coordinate system
            float X = FVector3d::DotProduct(ProjectedVertex, VX);
            float Y = FVector3d::DotProduct(ProjectedVertex, VY);
            ProjectedVertices.Push(FVector2D(X, Y));
            
            if (xAmount != 0) 
            {
                if (X < minX.X)
                {
                    minX = FVector2D(X, Y);
                    minIndex = VertexIndex;
                }
                else if (X > maxX.X)
                {
                    maxX = FVector2D(X, Y);
                    maxIndex = VertexIndex;
                }
            }
            if (yAmount != 0)
            {
                if (Y < minY.Y)
                {
                    minY = FVector2D(X, Y);
                    miyIndex = VertexIndex;
                }
                else if (Y > maxY.Y)
                {
                    maxY = FVector2D(X, Y);
                    mayIndex = VertexIndex;
                }
            }
        }
        
        int32 xDistance = maxX.X - minX.X;
        int32 yDistance = maxY.Y - minY.Y;

        for (int32 i = 0; i < ProjectedVertices.Num(); i++)
        {
            float xOffset = Amount * (1 - ((maxX.X - ProjectedVertices[i].X) / xDistance));
            float yOffset = Amount * (1 - ((maxY.Y - ProjectedVertices[i].Y) / yDistance));
            if (xAmount != 0)
            {
                Mesh->SetVertex(i, (Mesh->GetVertex(i) + xOffset * xAmount * NormalizedVector), true);
            }
            if (yAmount != 0)
            {
                Mesh->SetVertex(i, (Mesh->GetVertex(i) + yOffset * yAmount * NormalizedVector), true);
            }
        }
    }
}