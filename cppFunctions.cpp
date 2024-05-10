// Fill out your copyright notice in the Description page of Project Settings.


#include "cppFunctions.h"
#include <DocObj.h>
#include "Engine/GameEngine.h"
#include "Engine/World.h"
#include "Math/Quat.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"

bool UcppFunctions::test(UPrimitiveComponent* ComponentToSweep, const FVector& Start, const FVector& End, const FQuat& Rot, TArray<FHitResult>& OutHits, UWorld* World)
{

    FComponentQueryParams ComponentQueryParams;

    //ComponentQueryParams.AddIgnoredActor(ActorToIgnore);
    bool bHit = World->ComponentSweepMulti(
        OutHits,
        ComponentToSweep,
        Start,
        End,
        Rot,
        ComponentQueryParams
    );

    return bHit;
}