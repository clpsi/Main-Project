// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncPMC.h"

UAsyncPMC::UAsyncPMC(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bUseAsyncCooking = true;
}