#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"


class FVivaEngineModule : public FDefaultGameModuleImpl
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:

#if WITH_EDITOR
    // Declare any editor-specific functions or member variables here
#endif

};