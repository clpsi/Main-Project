// Fill out your copyright notice in the Description page of Project Settings.

#include "cppFunctions.h"
#include "Polygroups/PolygroupSet.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicAttribute.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Polygroups/PolygroupsGenerator.h"
#include "GeometryScript/MeshPolygroupFunctions.h"
#include "UDynamicMesh.h"
#include "GeometryScript/GeometryScriptTypes.h"
#include "IndexTypes.h"
#include "Engine/GameEngine.h"

#include <DocObj.h>
#include "Engine/World.h"
#include "Math/Quat.h"
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