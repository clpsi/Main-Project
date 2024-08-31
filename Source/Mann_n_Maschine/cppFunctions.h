// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Polygroups/PolygroupSet.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicAttribute.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMesh3.h"

#include "cppFunctions.generated.h"

/**
 * 
 */
UCLASS()
class MANN_N_MASCHINE_API UcppFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	static void MyGetTrianglesInPolygroup(UDynamicMeshComponent* DynamicMeshComp, int32 TriangleID, TArray<int32>& OutTriangleIndices, TArray<int32>& OutVertexIndices);
};
