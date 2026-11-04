
#include "Weather_BPL.h"
#include "WeatherNoiseShader.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHICommandList.h"

#include "CloudGenerationShader.h"
#include "RHIStaticStates.h"

void UWeatherBPL::DispatchPressureField(
    UTextureRenderTarget2D* Output,
    FVector2D WorldOrigin,
    float CellSize,
    float SimulationTime,
    FVector2D PatternVelocity,
    int32 Seed,
    int32 BaseCells,
    int32 Octaves,
    float Persistence,

    float WindStrength,
    float RotationStrength,
    float MaxWindStrength,

    float GustStrength,
    float GustScale
)
{
    if (!Output)
    {
        return;
    }

    if (BaseCells <= 0 ||
        Octaves <= 0 ||
        CellSize <= 0.0f ||
        MaxWindStrength <= 0.0f ||
        GustScale <= 0.0f)
    {
        return;
    }

    const FIntPoint Resolution(
        Output->SizeX,
        Output->SizeY
    );

    FTextureRenderTargetResource* Resource =
        Output->GameThread_GetRenderTargetResource();

    if (!Resource)
    {
        return;
    }

    const FVector2f Origin(
        (float)WorldOrigin.X,
        (float)WorldOrigin.Y
    );

    const FVector2f Velocity(
        (float)PatternVelocity.X,
        (float)PatternVelocity.Y
    );

    const float SafePersistence = FMath::Clamp(
        Persistence,
        0.0f,
        1.0f
    );

    const uint32 SafeSeed = (uint32)Seed;

    const int32 SafeOctaves = FMath::Clamp(
        Octaves,
        1,
        12
    );

    ENQUEUE_RENDER_COMMAND(DispatchWeatherNoise)(
        [
            Resource,
                Resolution,
                Origin,
                CellSize,
                SimulationTime,
                Velocity,
                SafeSeed,
                BaseCells,
                SafeOctaves,
                SafePersistence,

                WindStrength,
                RotationStrength,
                MaxWindStrength,

                GustStrength,
                GustScale
        ](FRHICommandListImmediate& RHICmdList)
            {
                FRDGBuilder GraphBuilder(RHICmdList);

                // Register combined weather texture

                TRefCountPtr<IPooledRenderTarget> ExternalTarget =
                    CreateRenderTarget(
                        Resource->GetRenderTargetTexture(),
                        TEXT("WeatherData")
                    );

                FRDGTextureRef OutputTexture =
                    GraphBuilder.RegisterExternalTexture(
                        ExternalTarget
                    );

                // Get compute shader

                TShaderMapRef<FWeatherNoiseCS> ComputeShader(
                    GetGlobalShaderMap(GMaxRHIFeatureLevel)
                );

                // Shader parameters

                FWeatherNoiseCS::FParameters* Parameters =
                    GraphBuilder.AllocParameters<
                    FWeatherNoiseCS::FParameters
                    >();

                Parameters->OutputSize = FUintVector2(
                    Resolution.X,
                    Resolution.Y
                );

                Parameters->WorldOrigin = Origin;

                Parameters->CellSize = CellSize;

                Parameters->SimulationTime = SimulationTime;

                Parameters->PatternVelocity = Velocity;

                Parameters->Seed = SafeSeed;

                Parameters->BaseCells = BaseCells;

                Parameters->Octaves = SafeOctaves;

                Parameters->Persistence = SafePersistence;

                // Wind

                Parameters->WindStrength = WindStrength;

                Parameters->RotationStrength = RotationStrength;

                Parameters->MaxWindStrength = MaxWindStrength;

                // Storm

                Parameters->GustStrength = GustStrength;

                Parameters->GustScale = GustScale;

                // Combined output UAV

                Parameters->OutputWeather =
                    GraphBuilder.CreateUAV(OutputTexture);

                // Dispatch dimensions

                const FIntVector GroupCount(
                    FMath::DivideAndRoundUp(Resolution.X, 8),
                    FMath::DivideAndRoundUp(Resolution.Y, 8),
                    1
                );

                FComputeShaderUtils::AddPass(
                    GraphBuilder,
                    RDG_EVENT_NAME("Weather Simulation"),
                    ComputeShader,
                    Parameters,
                    GroupCount
                );

                GraphBuilder.Execute();
            }
            );
}


void UWeatherBPL::DispatchCloudGeneration(
    UTextureRenderTarget2D* WeatherData,
    UTextureRenderTarget2D* Output,

    FLinearColor Coverage,
    FLinearColor Density,

    FLinearColor HumidityInfluence,
    FLinearColor PressureInfluence
)
{
    // ========================================================
    // VALIDATION
    // ========================================================

    if (!WeatherData || !Output)
    {
        return;
    }

    const FIntPoint Resolution(
        Output->SizeX,
        Output->SizeY
    );

    if (Resolution.X <= 0 || Resolution.Y <= 0)
    {
        return;
    }

    FTextureRenderTargetResource* WeatherResource =
        WeatherData->GameThread_GetRenderTargetResource();

    FTextureRenderTargetResource* OutputResource =
        Output->GameThread_GetRenderTargetResource();

    if (!WeatherResource || !OutputResource)
    {
        return;
    }


    // ========================================================
    // CONVERT PARAMETERS
    // ========================================================

    const FVector4f SafeCoverage(
        Coverage.R,
        Coverage.G,
        Coverage.B,
        Coverage.A
    );

    const FVector4f SafeDensity(
        Density.R,
        Density.G,
        Density.B,
        Density.A
    );

    const FVector4f SafeHumidityInfluence(
        HumidityInfluence.R,
        HumidityInfluence.G,
        HumidityInfluence.B,
        HumidityInfluence.A
    );

    const FVector4f SafePressureInfluence(
        PressureInfluence.R,
        PressureInfluence.G,
        PressureInfluence.B,
        PressureInfluence.A
    );


    // ========================================================
    // RENDER THREAD DISPATCH
    // ========================================================

    ENQUEUE_RENDER_COMMAND(DispatchCloudGeneration)(
        [
            WeatherResource,
                OutputResource,
                Resolution,

                SafeCoverage,
                SafeDensity,
                SafeHumidityInfluence,
                SafePressureInfluence

        ](FRHICommandListImmediate& RHICmdList)
        {
            FRDGBuilder GraphBuilder(RHICmdList);


            // ------------------------------------------------
            // 1. REGISTER WEATHER INPUT
            // ------------------------------------------------

            TRefCountPtr<IPooledRenderTarget> WeatherExternal =
                CreateRenderTarget(
                    WeatherResource->GetRenderTargetTexture(),
                    TEXT("WeatherData")
                );

            FRDGTextureRef WeatherTexture =
                GraphBuilder.RegisterExternalTexture(
                    WeatherExternal
                );


            // ------------------------------------------------
            // 2. REGISTER CLOUD OUTPUT
            // ------------------------------------------------

            TRefCountPtr<IPooledRenderTarget> CloudExternal =
                CreateRenderTarget(
                    OutputResource->GetRenderTargetTexture(),
                    TEXT("CloudData")
                );

            FRDGTextureRef CloudTexture =
                GraphBuilder.RegisterExternalTexture(
                    CloudExternal
                );


            // ------------------------------------------------
            // 3. GET COMPUTE SHADER
            // ------------------------------------------------

            TShaderMapRef<FCloudGenerationCS> ComputeShader(
                GetGlobalShaderMap(GMaxRHIFeatureLevel)
            );


            // ------------------------------------------------
            // 4. SHADER PARAMETERS
            // ------------------------------------------------

            FCloudGenerationCS::FParameters* Parameters =
                GraphBuilder.AllocParameters<
                FCloudGenerationCS::FParameters
                >();

            Parameters->OutputSize = FUintVector2(
                Resolution.X,
                Resolution.Y
            );

            Parameters->Coverage = SafeCoverage;

            Parameters->Density = SafeDensity;

            Parameters->HumidityInfluence =
                SafeHumidityInfluence;

            Parameters->PressureInfluence =
                SafePressureInfluence;

            Parameters->WeatherData = WeatherTexture;

            Parameters->OutputCloud =
                GraphBuilder.CreateUAV(CloudTexture);


            // ------------------------------------------------
            // 5. DISPATCH
            // ------------------------------------------------

            const FIntVector GroupCount(
                FMath::DivideAndRoundUp(Resolution.X, 8),
                FMath::DivideAndRoundUp(Resolution.Y, 8),
                1
            );

            FComputeShaderUtils::AddPass(
                GraphBuilder,
                RDG_EVENT_NAME("Cloud Generation"),
                ComputeShader,
                Parameters,
                GroupCount
            );


            // ------------------------------------------------
            // 6. EXECUTE
            // ------------------------------------------------

            GraphBuilder.Execute();
        }
            );
}
