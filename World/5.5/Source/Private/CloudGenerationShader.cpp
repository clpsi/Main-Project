
#include "CloudGenerationShader.h"

IMPLEMENT_GLOBAL_SHADER(
    FCloudGenerationCS,
    "/Project/Private/CloudGeneration.usf",
    "MainCS",
    SF_Compute
);
