#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions; 

class FVivaEngineModule : public FDefaultGameModuleImpl
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:

};