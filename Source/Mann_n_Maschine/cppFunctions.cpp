// Fill out your copyright notice in the Description page of Project Settings.

#include "cppFunctions.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "Components/DynamicMeshComponent.h"
#include "Components/StaticMeshComponent.h"

#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicAttribute.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshEditor.h"
#include "DynamicMesh/InfoTypes.h"

#include "RealtimeMeshActor.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMesh.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshByStreamBuilder.h"
#include "Core/RealtimeMeshBuilder.h"
#include "Core/RealtimeMeshDataStream.h"

#include "MeshBoundaryLoops.h"
#include "MeshDescription.h"

#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include "Polygroups/PolygroupSet.h"
#include "Polygroups/PolygroupsGenerator.h"

#include "GeometryScript/MeshPolygroupFunctions.h"
#include "GeometryScript/GeometryScriptTypes.h"

#include "GameFramework/Actor.h"
#include "Engine/GameEngine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Util/CompactMaps.h"
#include "StaticMeshAttributes.h"

#include "Misc/PackageName.h"
#include <DocObj.h>
#include "IndexTypes.h"

#include "Math/Quat.h"
#include "Math/Vector.h"
#include "Containers/Array.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
//D:\Programs\UE_5.4\Engine\Plugins\Runtime\GeometryScripting\Source\GeometryScriptingCore\Private



bool UcppFunctions::CopyPolygroupToMesh(UDynamicMeshComponent* InputMesh, UDynamicMeshComponent* TargetMesh, int32 Polygroup)
{
    if (InputMesh && InputMesh->GetMesh() && TargetMesh && TargetMesh->GetMesh())
    {
        const UE::Geometry::FDynamicMesh3* ReadMesh = InputMesh->GetMesh();

        UE::Geometry::FPolygroupLayer InputGroupLayer;
        InputGroupLayer.bIsDefaultLayer = true;
        if (InputGroupLayer.CheckExists(ReadMesh) == false)
        {
            //print error
            return false;
        }

        UE::Geometry::FPolygroupSet Groups(ReadMesh, InputGroupLayer);

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
        /*GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Vertices Count: %d"), count));
        UE_LOG(LogBlueprintUserMessages, Log, TEXT("Vertices Count: %d"), count);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Triangle Count: %d"), triangle));
        UE_LOG(LogBlueprintUserMessages, Log, TEXT("Triangle Count: %d"), triangle);*/
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

void UcppFunctions::NumPolygroups(UDynamicMeshComponent* InputMesh, TArray<int32>& PolygroupIDs)
{
    if (InputMesh)
    {
        UE::Geometry::FPolygroupLayer InputGroupLayer;
        InputGroupLayer.bIsDefaultLayer = true;
        FDynamicMesh3* Mesh = InputMesh->GetMesh();
        Mesh->EnableAttributes();
        UE::Geometry::FPolygroupSet Groups(Mesh, InputGroupLayer);
        
        for (int32 TriangleID : Mesh->TriangleIndicesItr())
        {   // Get the group ID for this triangle
            int32 GroupID = Groups.GetGroup(TriangleID);
            PolygroupIDs.Push(GroupID);
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
    UE::Geometry::FDynamicMeshEditor Editor(Mesh);
    UE::Geometry::FMeshBoundaryLoops BoundaryLoops(Mesh, true);

    if (BoundaryLoops.Compute())
    {
        for (const UE::Geometry::FEdgeLoop& Loop : BoundaryLoops.Loops)
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
            UE::Geometry::FDynamicMeshEditResult EditResult;

            // Fill the hole using AddTriangleFan_OrderedVertexLoop
            Editor.AddTriangleFan_OrderedVertexLoop(NewVertexID, Loop.Vertices, FDynamicMesh3::InvalidID, EditResult);
        }
    }


    // Update the mesh after modification
    MeshComponent->NotifyMeshUpdated();
}

void UcppFunctions::MoveVertices(UDynamicMeshComponent* InputMesh, float Amount, int32 xAmount, int32 yAmount)
{
    if (InputMesh)
    {
        UE::Geometry::FPolygroupLayer InputGroupLayer;
        InputGroupLayer.bIsDefaultLayer = true;
        FDynamicMesh3* Mesh = InputMesh->GetMesh();

        if (InputGroupLayer.CheckExists(Mesh) == false || Amount == 0.0)
        {
            UE_LOG(LogBlueprintUserMessages, Log, TEXT("Failed"));
        }

        Mesh->EnableAttributes();
        UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = Mesh->Attributes()->PrimaryNormals();
        TArray<FVector2D> ProjectedVertices;
        TArray<FVector3d> TriangleNormals;
        TArray<int32> VertexIndices;

        for (int i : Mesh->VertexIndicesItr())
        {
            VertexIndices.Push(i);
        }

        if (xAmount != 0 || yAmount != 0)
        {
            for (int32 TriangleID = 0; TriangleID < Mesh->MaxTriangleID(); ++TriangleID)
            {
                if (Mesh->IsTriangle(TriangleID))
                {
                    FVector3d Normal = Mesh->GetTriNormal(TriangleID);
                    TriangleNormals.Add(Normal);
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
                AverageNormal = (NormalSum / static_cast<double>(TriangleNormals.Num()));
            }

            AverageNormal.Normalize();
            FVector3d NormalizedVector = AverageNormal.GetSafeNormal(); //Plane Normal
            // Choose an arbitrary vector that is not parallel to NormalVector
            FVector3d ArbitraryVector = (FMath::Abs(NormalizedVector.X) < FMath::Abs(NormalizedVector.Y) &&
                FMath::Abs(NormalizedVector.X) < FMath::Abs(NormalizedVector.Z)) ?
                FVector3d(1, 0, 0) : ((FMath::Abs(NormalizedVector.Y) < FMath::Abs(NormalizedVector.Z)) ?
                FVector3d(0, 1, 0) : FVector3d(0, 0, 1));
            // Compute the first orthogonal vector using the cross product
            FVector3d VX = FVector3d::CrossProduct(NormalizedVector, ArbitraryVector);
            VX.Normalize();
            // Compute the second orthogonal vector using the cross product again
            FVector3d VY = FVector3d::CrossProduct(NormalizedVector, VX);
            VY.Normalize();

            float maxX = -100000.0f;
            float minX = 100000.0f;
            float maxY = -100000.0f;
            float minY = 100000.0f;

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
                    if (X < minX)
                    {
                        minX = X;
                    }
                    else if (X > maxX)
                    {
                        maxX = X;
                    }
                }
                if (yAmount != 0)
                {
                    if (Y < minY)
                    {
                        minY = Y;
                    }
                    else if (Y > maxY)
                    {
                        maxY = Y;
                    }
                }
            }

            int32 xDistance = maxX - minX;
            int32 yDistance = maxY - minY;

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
                X = xAmount != 0 ? (1.0 - (ProjectedVertices[VertexIndex].X - minX) / xDistance) : (1.0 - (ProjectedVertices[VertexIndex].Y - minY) / yDistance);
                Y = yAmount == 0 ? (1.0 - (ProjectedVertices[VertexIndex].X - minX) / xDistance) : (1.0 - (ProjectedVertices[VertexIndex].Y - minY) / yDistance);
                X = FMath::Sign(xAmount) >= 0 ? X : 1.0 - X;
                Y = FMath::Sign(yAmount) >= 0 ? Y : 1.0 - Y;
                ProjectedVertices[VertexIndex] = FVector2D(X, Y);
            }
        }

        for (int32 i = 0; i < VertexIndices.Num(); i++)
        {
            FVector3f Normal;
            NormalOverlay->GetElement(i, Normal);
            FVector3d NormalDouble = FVector3d(Normal);
            float X = xAmount != 0 || yAmount != 0 ? (ProjectedVertices[i].X + ProjectedVertices[i].Y) / 2.0 : 1.0;
            Mesh->SetVertex(i, (Mesh->GetVertex(i) + X * Amount * NormalDouble), true);
        }
    }
}

void UcppFunctions::CalculateSurface(UDynamicMeshComponent* InputMesh)
{
    float TotalSurfaceArea = 0.0f;
    FDynamicMesh3* Mesh = InputMesh->GetMesh();

    // Iterate through all triangles in the mesh
    for (int TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount(); ++TriangleIndex)
    {
        // Get the vertices of the triangle
        FVector3d VertexA, VertexB, VertexC;
        Mesh->GetTriVertices(TriangleIndex, VertexA, VertexB, VertexC);

        // Calculate two edge vectors
        FVector3d Edge1 = VertexB - VertexA;
        FVector3d Edge2 = VertexC - VertexA;

        // Compute the cross product of the edge vectors
        FVector3d CrossProduct = FVector3d::CrossProduct(Edge1, Edge2);

        // Calculate the area of the triangle (half of the magnitude of the cross product)
        float TriangleArea = CrossProduct.Length() * 0.5f;

        // Add the triangle's area to the total
        TotalSurfaceArea += TriangleArea;
    }

    //now trace along normal to find thickness for weight calculation
}

void UcppFunctions::SaveStaticMesh(UStaticMesh* InputStaticMesh, const FString& AssetName, const FString& PackagePath)
{
    if (!InputStaticMesh)
    {
        return; // Ensure the mesh to copy is valid
    }

    // Duplicate the Static Mesh
    UStaticMesh* StaticMesh = DuplicateObject<UStaticMesh>(InputStaticMesh, GetTransientPackage());

    FString PackageName = PackagePath + AssetName;
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package) return;

    if (FPackageName::DoesPackageExist(PackageName + ".uasset"))
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset package already exists: %s"), *PackageName);
        return;
    }

    StaticMesh->Rename(*AssetName, Package);

    // Ensure LOD0 Source Model exists and set BuildSettings
    if (StaticMesh->GetNumSourceModels() == 0)
    {
        StaticMesh->AddSourceModel();
    }
    FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(0);
    SourceModel.BuildSettings.bRecomputeNormals = true;
    SourceModel.BuildSettings.bRecomputeTangents = true;

    // Create and populate FMeshDescription for LOD0 if necessary
    FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0);
    if (!MeshDescription)
    {
        MeshDescription = StaticMesh->CreateMeshDescription(0);
        if (!MeshDescription)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create a valid MeshDescription for StaticMesh"));
            return;
        }
    }

    // Add vertices and triangle data to MeshDescription here if needed
    // (This step is essential; without vertex/triangle data, the mesh remains incomplete.)

    StaticMesh->CommitMeshDescription(0);

    // Build and initialize resources to validate mesh data
    StaticMesh->Build(true);
    StaticMesh->InitResources();

    // Register with AssetRegistry
    FAssetRegistryModule::AssetCreated(StaticMesh);
    StaticMesh->MarkPackageDirty();

    FString PackageFilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, StaticMesh, *PackageFilePath, SaveArgs);

    //destroy copiedMesh perhaps
}

URealtimeMeshSimple* UcppFunctions::ConvertToRMC(UObject* WorldContextObject, UDynamicMeshComponent* DynamicMeshComp)
{
    if (!WorldContextObject || !DynamicMeshComp || !DynamicMeshComp->GetDynamicMesh())
    {
        return nullptr;
    }

    const FDynamicMesh3* SourceDynamicMesh = DynamicMeshComp->GetDynamicMesh()->GetMeshPtr();
    if (!SourceDynamicMesh)
    {
        return nullptr;
    }

    // Create a new RealtimeMesh object
    URealtimeMeshSimple* RealtimeMesh = NewObject<URealtimeMeshSimple>(WorldContextObject);
    if (!RealtimeMesh)
    {
        return nullptr;
    }

    // Prepare StreamSet and Builder for RealtimeMesh
    RealtimeMesh::FRealtimeMeshStreamSet StreamSet;
    RealtimeMesh::TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
    Builder.EnableTangents();
    Builder.EnableTexCoords();
    Builder.EnableColors();

    // Get DynamicMesh attributes
    const UE::Geometry::FDynamicMeshAttributeSet* Attributes = SourceDynamicMesh->Attributes();
    const UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = Attributes ? Attributes->PrimaryNormals() : nullptr;
    const UE::Geometry::FDynamicMeshNormalOverlay* TangentOverlay = Attributes ? Attributes->PrimaryTangents() : nullptr;
    const UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = Attributes ? Attributes->PrimaryUV() : nullptr;
    const UE::Geometry::FDynamicMeshColorOverlay* ColorOverlay = Attributes ? Attributes->PrimaryColors() : nullptr;


    // Iterate through dynamic mesh triangles to build RealtimeMesh
    for (int32 TriID : SourceDynamicMesh->TriangleIndicesItr())
    {
        const UE::Geometry::FIndex3i DynamicMeshTri = SourceDynamicMesh->GetTriangle(TriID);

        // Add 3 vertices per triangle (one for each corner)
        for (int32 Corner = 0; Corner < 3; ++Corner)
        {
            const FVertexID VertexID = DynamicMeshTri[Corner];

            // Get position
            const FVector3f Position = (FVector3f)SourceDynamicMesh->GetVertex(VertexID);

            // Get Normal
            FVector3f Normal = FVector3f::UpVector; // Default value
            if (NormalOverlay) // && NormalOverlay->IsTriangle(TriID) should be EnumerateVertexElements(TriID, ?, false)
            {
                Normal = (FVector3f)NormalOverlay->GetElement(NormalOverlay->GetTriangle(TriID)[Corner]);
            }

            // Get Tangent
            FVector3f Tangent = FVector3f::RightVector; // Default value
            if (TangentOverlay)
            {
                Tangent = (FVector3f)TangentOverlay->GetElement(TangentOverlay->GetTriangle(TriID)[Corner]);
            }

            // Get UV
            FVector2f UV = FVector2f::ZeroVector; // Default value
            if (UVOverlay)
            {
                UV = (FVector2f)UVOverlay->GetElement(UVOverlay->GetTriangle(TriID)[Corner]);
            }

            // Get Color
            FLinearColor Color = FLinearColor::White; // Default value
            if (ColorOverlay)
            {
                Color = (FLinearColor)ColorOverlay->GetElement(ColorOverlay->GetTriangle(TriID)[Corner]);
            }


            // Add vertex to RealtimeMesh builder
            // The builder will internally handle creating unique vertices based on attributes
            const int32 RealtimeMeshVertexIndex = Builder.AddVertex(Position)
                .SetNormalAndTangent(Normal, Tangent)
                .SetTexCoord(UV)
                .SetColor(Color)
                .GetIndex();

            // This is used for updating existing triangle, which there are none yet
            //Builder.SetTriangle(Builder.NumTriangles(), RealtimeMeshVertexIndex, RealtimeMeshVertexIndex, RealtimeMeshVertexIndex); // Placeholder, will be overwritten
        }

        // Add the triangle using the last three added vertices
        Builder.AddTriangle(
            Builder.NumVertices() - 3,
            Builder.NumVertices() - 2,
            Builder.NumVertices() - 1
        );
    }
    // Set material slot (assuming a single material)
    const int32 MaterialSlot = 0;

    // Create a section group in RealtimeMesh
    const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(0, "ConvertedDynamicMesh");
    RealtimeMesh->CreateSectionGroup(SectionGroupKey, StreamSet);

    // Enable collision for the first section (polygroup 0)
    const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 0);
    RealtimeMesh->UpdateSectionConfig(SectionKey, FRealtimeMeshSectionConfig(), true);

    // Setup collision configuration
    FRealtimeMeshCollisionConfiguration CollisionConfig;
    CollisionConfig.bUseComplexAsSimpleCollision = true;
    RealtimeMesh->SetCollisionConfig(CollisionConfig);

    return RealtimeMesh;
}

void UcppFunctions::ConvertToDMC(UDynamicMeshComponent* DynamicMeshComp, URealtimeMeshSimple* RealtimeMesh)
{
    if (!DynamicMeshComp || !RealtimeMesh)
    {
        return;
    }

    // Access RealtimeMesh data
    RealtimeMesh::FRealtimeMeshStreamSet StreamSet;

    // Get the first LOD and first section group for processing
    TArray<FRealtimeMeshLODKey> LODKeys = RealtimeMesh->GetLODs();
    if (LODKeys.Num() == 0)
    {
        return;
    }
    FRealtimeMeshLODKey FirstLODKey = LODKeys[0];

    TArray<FRealtimeMeshSectionGroupKey> SectionGroupKeys = RealtimeMesh->GetSectionGroups(FirstLODKey);
    if (SectionGroupKeys.Num() == 0)
    {
        return;
    }
    FRealtimeMeshSectionGroupKey FirstSectionGroupKey = SectionGroupKeys[0];

    RealtimeMesh->ProcessMesh(FirstSectionGroupKey, [&](const RealtimeMesh::FRealtimeMeshStreamSet& InStreamSet)
        {
            StreamSet.CopyFrom(InStreamSet); // Explicitly copy the stream set for local processing
        }
    );

    if (StreamSet.IsEmpty())
    {
        return;
    }

    // Get position, triangle, normal, tangent, and UV streams
    const RealtimeMesh::FRealtimeMeshStream* PositionStream = StreamSet.Find(RealtimeMesh::FRealtimeMeshStreams::Position);
    const RealtimeMesh::FRealtimeMeshStream* TriangleStream = StreamSet.Find(RealtimeMesh::FRealtimeMeshStreams::Triangles);
    const RealtimeMesh::FRealtimeMeshStream* TangentsStream = StreamSet.Find(RealtimeMesh::FRealtimeMeshStreams::Tangents);
    const RealtimeMesh::FRealtimeMeshStream* TexCoordsStream = StreamSet.Find(RealtimeMesh::FRealtimeMeshStreams::TexCoords);
    const RealtimeMesh::FRealtimeMeshStream* ColorStream = StreamSet.Find(RealtimeMesh::FRealtimeMeshStreams::Color);


    if (!PositionStream || !TriangleStream)
    {
        return;
    }

    // Begin editing the DynamicMesh
    DynamicMeshComp->EditMesh([&](FDynamicMesh3& EditMesh)
    {
            EditMesh.Clear();

            TMap<int32, FVertexID> RealtimeMeshToDynamicMeshVID;
            RealtimeMeshToDynamicMeshVID.Reserve(PositionStream->Num());

            // Add Vertices
            for (int32 i = 0; i < PositionStream->Num(); ++i)
            {
                FVector3f Position = PositionStream->GetDataAtVertex<FVector3f>(i)[0];
                RealtimeMeshToDynamicMeshVID.Add(i, EditMesh.AppendVertex((FVector3d)Position));
            }

            // Add Triangles
            RealtimeMesh::TRealtimeMeshStreamBuilder<const RealtimeMesh::TIndex3<uint32>> TriangleBuilder(*TriangleStream);
            for (int32 i = 0; i < TriangleBuilder.Num(); ++i)
            {
                const RealtimeMesh::TIndex3<uint32> RMTri = TriangleBuilder[i];
                EditMesh.AppendTriangle(
                    RealtimeMeshToDynamicMeshVID[RMTri.V0],
                    RealtimeMeshToDynamicMeshVID[RMTri.V1],
                    RealtimeMeshToDynamicMeshVID[RMTri.V2]
                );
            }

            // Handle Attributes
            if (TangentsStream || TexCoordsStream || ColorStream)
            {
                EditMesh.EnableAttributes();

                UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = nullptr;
                UE::Geometry::FDynamicMeshNormalOverlay* TangentOverlay = nullptr;
                UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = nullptr;
                UE::Geometry::FDynamicMeshColorOverlay* ColorOverlay = nullptr;


                if (TangentsStream)
                {
                    NormalOverlay = EditMesh.Attributes()->PrimaryNormals();
                    TangentOverlay = EditMesh.Attributes()->PrimaryTangents();
                    //NormalOverlay->SetNumLayers(1); // Ensure primary layer exists
                    //TangentOverlay->SetNumLayers(1); // Ensure primary layer exists
                    EditMesh.Attributes()->EnableTangents(); // Normals + Tangents
                }
                if (TexCoordsStream)
                {
                    UVOverlay = EditMesh.Attributes()->PrimaryUV();
                    //UVOverlay->SetNumLayers(TexCoordsStream->GetNumElements()); // Match number of UV channels
                }
                if (ColorStream)
                {
                    ColorOverlay = EditMesh.Attributes()->PrimaryColors();
                    //ColorOverlay->SetNumLayers(1); // Ensure primary layer exists
                    EditMesh.Attributes()->EnablePrimaryColors();
                }

                // Iterate through RealtimeMesh triangles to set attributes
                RealtimeMesh::TRealtimeMeshStreamBuilder<const RealtimeMesh::TIndex3<uint32>> RMTriangleBuilder(*TriangleStream);
                for (int32 TriIdx = 0; TriIdx < RMTriangleBuilder.Num(); ++TriIdx)
                {
                    const RealtimeMesh::TIndex3<uint32> RMTri = RMTriangleBuilder[TriIdx];

                    FVector3d CornerNormals[3];
                    FVector3d CornerTangents[3];
                    FVector2d CornerUVs[3];
                    FVector4f CornerColors[3];


                    for (int32 Corner = 0; Corner < 3; ++Corner)
                    {
                        const int32 RMVertexIndex = RMTri[Corner];

                        // Get Normal and Tangent from RealtimeMesh
                        if (TangentsStream)
                        {
                            const RealtimeMesh::TRealtimeMeshTangents<FPackedNormal> RMTangents = TangentsStream->GetDataAtVertex<RealtimeMesh::TRealtimeMeshTangents<FPackedNormal>>(RMVertexIndex)[0];
                            CornerNormals[Corner] = (FVector3d)RMTangents.GetNormal();
                            CornerTangents[Corner] = (FVector3d)RMTangents.GetTangent();
                        }
                        else
                        {
                            CornerNormals[Corner] = FVector3d::UpVector;
                            CornerTangents[Corner] = FVector3d::RightVector;
                        }

                        // Get UV from RealtimeMesh
                        if (TexCoordsStream)
                        {
                            // Assuming only one UV channel for simplicity, extend for more if needed
                            const FVector2DHalf RMUV = TexCoordsStream->GetDataAtVertex<FVector2DHalf>(RMVertexIndex, 0)[0];
                            CornerUVs[Corner] = (FVector2d)RMUV;
                        }
                        else
                        {
                            CornerUVs[Corner] = FVector2d::Zero();
                        }

                        // Get Color from RealtimeMesh
                        if (ColorStream)
                        {
                            const FColor RMColor = ColorStream->GetDataAtVertex<FColor>(RMVertexIndex)[0];
                            CornerColors[Corner] = RMColor.ReinterpretAsLinear();
                        }
                        else
                        {
                            CornerColors[Corner] = FVector4f(FLinearColor::White);
                        }
                    }

                    if (NormalOverlay && TangentOverlay) {
                        NormalOverlay->SetTriangle(
                            TriIdx,
                            UE::Geometry::FIndex3i(
                                NormalOverlay->AppendElement(FVector3f(CornerNormals[0])),
                                NormalOverlay->AppendElement(FVector3f(CornerNormals[1])),
                                NormalOverlay->AppendElement(FVector3f(CornerNormals[2]))
                            ),
                            true
                        );

                        TangentOverlay->SetTriangle(
                            TriIdx,
                            UE::Geometry::FIndex3i(
                                TangentOverlay->AppendElement(FVector3f(CornerTangents[0])),
                                TangentOverlay->AppendElement(FVector3f(CornerTangents[1])),
                                TangentOverlay->AppendElement(FVector3f(CornerTangents[2]))
                            ),
                            true
                        );
                    } if (UVOverlay) {
                        UVOverlay->SetTriangle(
                            TriIdx,
                            UE::Geometry::FIndex3i(
                                UVOverlay->AppendElement(FVector2f(CornerUVs[0])),
                                UVOverlay->AppendElement(FVector2f(CornerUVs[1])),
                                UVOverlay->AppendElement(FVector2f(CornerUVs[2]))
                            ),
                            true
                        );
                    } if (ColorOverlay) {
                        ColorOverlay->SetTriangle(
                            TriIdx,
                            UE::Geometry::FIndex3i(
                                ColorOverlay->AppendElement(FVector4f(CornerColors[0])),
                                ColorOverlay->AppendElement(FVector4f(CornerColors[1])),
                                ColorOverlay->AppendElement(FVector4f(CornerColors[2]))
                            ),
                            true
                        );
                    }
                }
                UE::Geometry::FCompactMaps CompactInfo;
                EditMesh.CompactInPlace(&CompactInfo);
                EditMesh.Attributes()->CompactInPlace(CompactInfo);// Commit changes to attributes
            }
    }); // End EditMesh

    // Notify component that mesh has been modified
    DynamicMeshComp->NotifyMeshModified();
    DynamicMeshComp->UpdateCollision(true); // Rebuild collision if desired

}

