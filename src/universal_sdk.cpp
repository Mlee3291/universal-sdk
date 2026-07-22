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
        platform_impl_ = CreatePlatformImpl();
        if (!platform_impl_ || !platform_impl_->Initialize()) {
            return false;
        }

        // Detect platform
#ifdef _WIN32
        current_platform_ = Platform::WINDOWS;
#elif defined(__linux__) && !defined(__ANDROID__)
        current_platform_ = Platform::LINUX;
#elif defined(__ANDROID__)
        current_platform_ = Platform::ANDROID_OS;
#else
        current_platform_ = Platform::UNKNOWN;
        platform_impl_->Shutdown();
        platform_impl_.reset();
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
        if (platform_impl_) {
            platform_impl_->Shutdown();
            platform_impl_.reset();
        }
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
        case Platform::ANDROID_OS:
            return "Android";
        default:
            return "Unknown";
    }
}

std::string UniversalSDK::GetVersion() const {
    return UNIVERSAL_SDK_VERSION;
}

std::string UniversalSDK::GetSDKInfo() const {
    return "Universal SDK v" + GetVersion() + " - Platform: " + GetPlatformName();
}

bool UniversalSDK::ExecuteCommand(const std::string& command) {
    if (!initialized_ || !platform_impl_) {
        std::cerr << "SDK not initialized" << std::endl;
        return false;
    }
    return platform_impl_->ExecuteCommand(command);
}

std::string UniversalSDK::GetSystemInfo() {
    if (!initialized_ || !platform_impl_) {
        return "SDK not initialized";
    }
    return platform_impl_->GetSystemInfo();
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
    LegalAssistantConfig config,
    std::shared_ptr<AuditWriter> audit_writer) const {
    return std::make_unique<LegalAssistant>(
        std::move(llm_provider),
        std::move(retrieval_provider),
        std::move(data_connector),
        std::move(policy_engine),
        std::move(config),
        std::move(audit_writer));
}

} // namespace UniversalSDK