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
	static bool CopyPolygroupToMesh(UDynamicMeshComponent* InputMesh, UDynamicMeshComponent* TargetMesh, int32 Polygroup);
	UFUNCTION(BlueprintCallable)
	static bool SweepComponent(UPrimitiveComponent* ComponentToSweep, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, TArray<FHitResult>& OutHits);
	UFUNCTION(BlueprintCallable)
	static bool TraceFromInsideMesh(AActor* ActorToIgnore, bool ShouldIgnore, const FVector& Start, const FVector& End, FHitResult& OutHit);
	UFUNCTION(BlueprintCallable)
	static void EnableDoubleSidedGeometry(UDynamicMeshComponent* InputMesh);
	UFUNCTION(BlueprintCallable)
	static void FillHolesInDynamicMeshComponent(UDynamicMeshComponent* MeshComponent);
	UFUNCTION(BlueprintCallable)
	static void MoveVertices(UDynamicMeshComponent* InputMesh, int32 Polygroup, float Amount, int32 xAmount, int32 yAmount);
	UFUNCTION(BlueprintCallable)
	static void CalculateSurface(UDynamicMeshComponent* InputMesh);
	UFUNCTION(BlueprintCallable)
	static void SaveStaticMesh(UStaticMesh* Mesh, const FString& AssetName, const FString& PackagePath);
};
