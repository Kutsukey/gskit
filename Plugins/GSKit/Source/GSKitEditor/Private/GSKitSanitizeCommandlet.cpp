#include "GSKitSanitizeCommandlet.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "validation.hpp"
#include <filesystem>

DEFINE_LOG_CATEGORY_STATIC(LogGSKit, Display, All);

UGSKitSanitizeCommandlet::UGSKitSanitizeCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
    ShowErrorCount = false;
    UseCommandletResultAsExitCode = true;
}

int32 UGSKitSanitizeCommandlet::Main(const FString& Params)
{
    FString InputPath;
    FParse::Value(*Params, TEXT("Input="), InputPath);
    InputPath.TrimQuotesInline();

    if (InputPath.IsEmpty() || !FPaths::FileExists(InputPath))
    {
        UE_LOG(LogGSKit, Error, TEXT("[gskit] Usage: -run=GSKitSanitize -Input=<path.ply>"));
        return 2;
    }

    UE_LOG(LogGSKit, Display, TEXT("[gskit] Ingestion Firewall Running on: %s"), *InputPath);

    gskit::ValidationOptions Options;
    Options.dropDeadGaussians = true;

    std::filesystem::path NativePath(TCHAR_TO_UTF8(*InputPath));
    gskit::ValidationResult Result = gskit::ValidateAsset(NativePath, Options);

    UE_LOG(LogGSKit, Display, TEXT("[gskit] Splat Count: %llu | Errors: %u | Ghosts: %llu"),
        static_cast<unsigned long long>(Result.summary.gaussianCount),
        static_cast<unsigned int>(Result.summary.errorCount),
        static_cast<unsigned long long>(Result.summary.ghostCount));

    if (Result.summary.errorCount > 0)
    {
        UE_LOG(LogGSKit, Error, TEXT("[gskit] Build Rejected: Asset contains fatal anomalies."));
        return 2;
    }

    UE_LOG(LogGSKit, Display, TEXT("[gskit] Asset Validated Clean for UE5 Structured Buffers."));
    return 0;
}
