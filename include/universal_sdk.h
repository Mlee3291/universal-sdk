#ifndef UNIVERSAL_SDK_H
#define UNIVERSAL_SDK_H

#include <string>
#include <vector>
#include <memory>
#include "legal_assistant.h"

namespace UniversalSDK {

// Platform enumeration
enum class Platform {
    WINDOWS,
    LINUX,
    ANDROID,
    UNKNOWN
};

// Core SDK interface
class UniversalSDK {
public:
    UniversalSDK();
    ~UniversalSDK();

    // Initialization and cleanup
    bool Initialize();
    void Shutdown();

    // Platform detection
    Platform GetPlatform() const;
    std::string GetPlatformName() const;

    // Version information
    std::string GetVersion() const;
    std::string GetSDKInfo() const;

    // Platform-specific operations
    bool ExecuteCommand(const std::string& command);
    std::string GetSystemInfo();

    // Utility functions
    bool IsInitialized() const;
    std::vector<std::string> GetAvailableFeatures() const;

    // Legal-assistant module factory
    std::unique_ptr<LegalAssistant> CreateLegalAssistant(
        std::shared_ptr<LLMProvider> llm_provider,
        std::shared_ptr<RetrievalProvider> retrieval_provider,
        std::shared_ptr<InmateDataConnector> data_connector,
        std::shared_ptr<PolicyEngine> policy_engine,
        LegalAssistantConfig config = LegalAssistantConfig{}) const;

private:
    bool initialized_;
    Platform current_platform_;
};

// Platform-specific implementation interface
class PlatformImpl {
public:
    virtual ~PlatformImpl() = default;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual std::string GetSystemInfo() = 0;
    virtual bool ExecuteCommand(const std::string& command) = 0;
};

// Factory function to create platform-specific implementation
std::unique_ptr<PlatformImpl> CreatePlatformImpl();

} // namespace UniversalSDK

#endif // UNIVERSAL_SDK_H