#include "universal_sdk.h"
#include "legal_assistant.h"
#include <iostream>
#include <stdexcept>

namespace UniversalSDK {

UniversalSDK::UniversalSDK() 
    : initialized_(false), current_platform_(Platform::UNKNOWN) {
}

UniversalSDK::~UniversalSDK() {
    if (initialized_) {
        Shutdown();
    }
}

bool UniversalSDK::Initialize() {
    try {
        // Detect platform
#ifdef _WIN32
        current_platform_ = Platform::WINDOWS;
#elif defined(__linux__) && !defined(__ANDROID__)
        current_platform_ = Platform::LINUX;
#elif defined(__ANDROID__)
        current_platform_ = Platform::ANDROID;
#else
        current_platform_ = Platform::UNKNOWN;
        return false;
#endif
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

void UniversalSDK::Shutdown() {
    if (initialized_) {
        initialized_ = false;
    }
}

Platform UniversalSDK::GetPlatform() const {
    return current_platform_;
}

std::string UniversalSDK::GetPlatformName() const {
    switch (current_platform_) {
        case Platform::WINDOWS:
            return "Windows";
        case Platform::LINUX:
            return "Linux";
        case Platform::ANDROID:
            return "Android";
        default:
            return "Unknown";
    }
}

std::string UniversalSDK::GetVersion() const {
    return "1.0.0";
}

std::string UniversalSDK::GetSDKInfo() const {
    return "Universal SDK v" + GetVersion() + " - Platform: " + GetPlatformName();
}

bool UniversalSDK::ExecuteCommand(const std::string& command) {
    if (!initialized_) {
        std::cerr << "SDK not initialized" << std::endl;
        return false;
    }
    
    // Command execution will be delegated to platform-specific implementation
    // This is a placeholder
    return true;
}

std::string UniversalSDK::GetSystemInfo() {
    if (!initialized_) {
        return "SDK not initialized";
    }
    
    return "System running on: " + GetPlatformName();
}

bool UniversalSDK::IsInitialized() const {
    return initialized_;
}

std::vector<std::string> UniversalSDK::GetAvailableFeatures() const {
    return {
        "Platform Detection",
        "System Information",
        "Command Execution",
        "Cross-Platform Support",
        "Inmate Legal Assistance"
    };
}

std::unique_ptr<LegalAssistant> UniversalSDK::CreateLegalAssistant(
    std::shared_ptr<LLMProvider> llm_provider,
    std::shared_ptr<RetrievalProvider> retrieval_provider,
    std::shared_ptr<InmateDataConnector> data_connector,
    std::shared_ptr<PolicyEngine> policy_engine,
    LegalAssistantConfig config) const {
    return std::make_unique<LegalAssistant>(
        std::move(llm_provider),
        std::move(retrieval_provider),
        std::move(data_connector),
        std::move(policy_engine),
        std::move(config));
}

} // namespace UniversalSDK