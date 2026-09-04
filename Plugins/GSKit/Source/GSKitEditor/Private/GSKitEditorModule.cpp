#include "Modules/ModuleManager.h"

class FGSKitEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FGSKitEditorModule, GSKitEditor)
