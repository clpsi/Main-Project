
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Weather_BPL.generated.h"

UCLASS()
class LANDSCAPE_PROJ_API UWeatherBPL
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Simulation")
    static void DispatchPressureField(
        UTextureRenderTarget2D* Output,
        FVector2D WorldOrigin,
        float CellSize,
        float SimulationTime,
        FVector2D PatternVelocity,
        int32 Seed,
        int32 BaseCells,
        int32 Octaves,
        float Persistence,

        // Wind parameters

        float WindStrength,
        float RotationStrength,
        float MaxWindStrength,

        // Storm parameters

        float GustStrength,
        float GustScale
    );


    UFUNCTION(BlueprintCallable, Category = "Weather")
    static void DispatchCloudGeneration(
        UTextureRenderTarget2D* WeatherData,
        UTextureRenderTarget2D* Output,

        FLinearColor Coverage,
        FLinearColor Density,

        FLinearColor HumidityInfluence,
        FLinearColor PressureInfluence
    );

};
