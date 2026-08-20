// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "AsyncPMC.generated.h"

/**
 * 
 */
UCLASS()
class LANDSCAPE_PROJ_API UAsyncPMC : public UProceduralMeshComponent
{
	GENERATED_BODY()
	
public:

	UAsyncPMC(const FObjectInitializer& ObjectInitializer);
};
