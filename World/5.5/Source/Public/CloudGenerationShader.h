
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FCloudGenerationCS : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER(FCloudGenerationCS);
    SHADER_USE_PARAMETER_STRUCT(FCloudGenerationCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

        SHADER_PARAMETER(FUintVector2, OutputSize)

        // Independent cloud layer settings

        SHADER_PARAMETER(FVector4f, Coverage)

        SHADER_PARAMETER(FVector4f, Density)

        SHADER_PARAMETER(FVector4f, HumidityInfluence)

        SHADER_PARAMETER(FVector4f, PressureInfluence)

        // Weather input

        SHADER_PARAMETER_RDG_TEXTURE(
            Texture2D<float4>,
            WeatherData
        )

        // Cloud output

        SHADER_PARAMETER_RDG_TEXTURE_UAV(
            RWTexture2D<float4>,
            OutputCloud
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
