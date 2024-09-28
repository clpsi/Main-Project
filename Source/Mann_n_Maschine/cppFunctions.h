// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Polygroups/PolygroupSet.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicAttribute.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMesh3.h"

#include "Math/Quat.h"
#include "Engine/World.h"

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
	static bool CopyPolygroupToMesh(UDynamicMeshComponent* InputMeshComp, UDynamicMeshComponent* TargetMesh, int32 Polygroup);
	UFUNCTION(BlueprintCallable)
	static bool SweepComponent(UPrimitiveComponent* ComponentToSweep, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, TArray<FHitResult>& OutHits);
};
