
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FWeatherNoiseCS : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER(FWeatherNoiseCS);
    SHADER_USE_PARAMETER_STRUCT(FWeatherNoiseCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

        SHADER_PARAMETER(FUintVector2, OutputSize)

        SHADER_PARAMETER(FVector2f, WorldOrigin)

        SHADER_PARAMETER(float, CellSize)

        SHADER_PARAMETER(float, SimulationTime)

        SHADER_PARAMETER(FVector2f, PatternVelocity)

        SHADER_PARAMETER(uint32, Seed)

        SHADER_PARAMETER(int32, BaseCells)

        SHADER_PARAMETER(int32, Octaves)

        SHADER_PARAMETER(float, Persistence)

        // Wind parameters

        SHADER_PARAMETER(float, WindStrength)

        SHADER_PARAMETER(float, RotationStrength)

        SHADER_PARAMETER(float, MaxWindStrength)

        // Storm parameters

        SHADER_PARAMETER(float, GustStrength)

        SHADER_PARAMETER(float, GustScale)

        // Combined output

        SHADER_PARAMETER_RDG_TEXTURE_UAV(
            RWTexture2D<float4>,
            OutputWeather
        )

        END_SHADER_PARAMETER_STRUCT()

        static bool ShouldCompilePermutation(
            const FGlobalShaderPermutationParameters& Parameters
        )
    {
        return IsFeatureLevelSupported(
            Parameters.Platform,
            ERHIFeatureLevel::SM5
        );
    }
};
