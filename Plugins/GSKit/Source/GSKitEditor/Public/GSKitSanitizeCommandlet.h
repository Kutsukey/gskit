#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GSKitSanitizeCommandlet.generated.h"

UCLASS()
class GSKITEDITOR_API UGSKitSanitizeCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGSKitSanitizeCommandlet();
    virtual int32 Main(const FString& Params) override;
};
