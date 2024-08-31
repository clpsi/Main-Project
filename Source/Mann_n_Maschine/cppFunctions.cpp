// Fill out your copyright notice in the Description page of Project Settings.

#include "cppFunctions.h"
#include "Polygroups/PolygroupSet.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicAttribute.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "IndexTypes.h"
#include "Engine/GameEngine.h"


void UcppFunctions::MyGetTrianglesInPolygroup(UDynamicMeshComponent* DynamicMeshComp, int32 TriangleID, TArray<int32>& OutTriangleIndices, TArray<int32>& OutVertexIndices)
{
    OutTriangleIndices.Empty();
    OutVertexIndices.Empty();

    TSet<int32> UniqueVertices;

    if (DynamicMeshComp && DynamicMeshComp->GetMesh())
    {

        FDynamicMesh3* Mesh = DynamicMeshComp->GetMesh();
        UE::Geometry::FDynamicMeshAttributeSet* Attributes = Mesh->Attributes();
        if (Attributes->NumPolygroupLayers() == 0)
        {
            Attributes->SetNumPolygroupLayers(1);
        }
        UE::Geometry::FDynamicMeshPolygroupAttribute* PolygroupAttr = Mesh->Attributes()->GetPolygroupLayer(0);// Assuming first layer

        if (PolygroupAttr)
        {
            // Iterate over every triangle in the mesh
            int32 PolygroupID = PolygroupAttr->GetValue(TriangleID);
            for (int32 TriangleIndex : Mesh->TriangleIndicesItr())
            {
                int32 CurrentPolygroupID = PolygroupAttr->GetValue(TriangleIndex);

                // If the triangle's polygroup ID matches the target polygroup, add it to the list
                if (CurrentPolygroupID == PolygroupID)
                {
                    OutTriangleIndices.Add(TriangleIndex);

                    UE::Geometry::FIndex3i Vertices = Mesh->GetTriangle(TriangleIndex);
                    UniqueVertices.Add(Vertices.A);
                    UniqueVertices.Add(Vertices.B);
                    UniqueVertices.Add(Vertices.C);
                }
            }
        }

    }
    
    OutVertexIndices = UniqueVertices.Array();
}