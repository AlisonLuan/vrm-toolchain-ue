#include "VrmToolchainBPLibrary.h"
#include "VrmToolchainWrapper.h"

#include <vrm_validate/vrm_validator.hpp>

bool UVrmToolchainBPLibrary::ValidateVrmFile(const FString& FilePath, FString& OutStatus)
{
    std::string path = TCHAR_TO_UTF8(*FilePath);
    try {
        auto report = vrm::VRMValidator::ValidateFile(path);
        OutStatus = FString(report.result.summary.status.c_str());
        return report.result.valid;
    } catch (const std::exception& e) {
        OutStatus = FString(e.what());
        return false;
    } catch (...) {
        OutStatus = TEXT("unknown error");
        return false;
    }
}
