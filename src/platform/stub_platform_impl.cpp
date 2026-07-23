#include "universal_sdk.h"

namespace UniversalSDK {

// Stub implementation used on unrecognised platforms so the library always
// links successfully. UniversalSDK::Initialize() will still return false when
// the current_platform_ is UNKNOWN, but this avoids an unresolved symbol error.
class StubPlatformImpl : public PlatformImpl {
public:
    bool Initialize() override { return true; }
    void Shutdown() override {}
    std::string GetSystemInfo() override { return "Unknown platform"; }
    bool ExecuteCommand(const std::string& /*command*/) override { return false; }
};

std::unique_ptr<PlatformImpl> CreatePlatformImpl() {
    return std::make_unique<StubPlatformImpl>();
}

} // namespace UniversalSDK
