
#include "Landscape_proj.h"

#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

// Custom project module
class FLandscape_projModule : public FDefaultGameModuleImpl
{
public:

    virtual void StartupModule() override;
};

// Register our custom module
IMPLEMENT_PRIMARY_GAME_MODULE(
    FLandscape_projModule,
    Landscape_proj,
    "Landscape_proj"
);


// Module startup
void FLandscape_projModule::StartupModule()
{
    const FString ShaderDirectory =
        FPaths::Combine(
            FPaths::ProjectDir(),
            TEXT("Shaders")
        );

    AddShaderSourceDirectoryMapping(
        TEXT("/Project"),
        ShaderDirectory
    );
}
