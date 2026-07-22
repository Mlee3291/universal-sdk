#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "legal_assistant.h"
#include "stub_providers.h"
#include "universal_sdk.h"

// The class and its namespace share the same name; use a type alias to
// disambiguate while still benefiting from the namespace import for all other
// SDK types.
using namespace UniversalSDK;
using SDK = UniversalSDK::UniversalSDK;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::unique_ptr<SDK> MakeInitialized() {
    auto sdk = std::make_unique<SDK>();
    EXPECT_TRUE(sdk->Initialize());
    return sdk;
}

// ---------------------------------------------------------------------------
// Initialization and lifecycle
// ---------------------------------------------------------------------------

TEST(UniversalSDKTest, NotInitializedBeforeInit) {
    SDK sdk;
    EXPECT_FALSE(sdk.IsInitialized());
}

TEST(UniversalSDKTest, InitializeReturnsTrue) {
    SDK sdk;
    EXPECT_TRUE(sdk.Initialize());
}

TEST(UniversalSDKTest, IsInitializedAfterInit) {
    SDK sdk;
    sdk.Initialize();
    EXPECT_TRUE(sdk.IsInitialized());
}

TEST(UniversalSDKTest, ShutdownClearsInitialized) {
    SDK sdk;
    sdk.Initialize();
    sdk.Shutdown();
    EXPECT_FALSE(sdk.IsInitialized());
}

TEST(UniversalSDKTest, DestructorDoesNotCrashWhenInitialized) {
    // SDK::~SDK calls Shutdown() when initialized.
    {
        SDK sdk;
        sdk.Initialize();
    }  // destructor invoked here
}

// ---------------------------------------------------------------------------
// Platform detection
// ---------------------------------------------------------------------------

TEST(UniversalSDKTest, GetPlatformReturnsKnownPlatform) {
    auto sdk = MakeInitialized();
    const Platform p = sdk->GetPlatform();
    EXPECT_NE(p, Platform::UNKNOWN);
}

TEST(UniversalSDKTest, GetPlatformMatchesCompileTimeDetection) {
    auto sdk = MakeInitialized();
#if defined(_WIN32)
    EXPECT_EQ(sdk->GetPlatform(), Platform::WINDOWS);
#elif defined(__ANDROID__)
    EXPECT_EQ(sdk->GetPlatform(), Platform::ANDROID);
#elif defined(__linux__)
    EXPECT_EQ(sdk->GetPlatform(), Platform::LINUX);
#endif
}

TEST(UniversalSDKTest, GetPlatformNameIsNonEmptyAndNotUnknown) {
    auto sdk = MakeInitialized();
    const std::string name = sdk->GetPlatformName();
    EXPECT_FALSE(name.empty());
    EXPECT_NE(name, "Unknown");
}

// ---------------------------------------------------------------------------
// Version and info
// ---------------------------------------------------------------------------

TEST(UniversalSDKTest, GetVersionIsNonEmpty) {
    SDK sdk;
    EXPECT_FALSE(sdk.GetVersion().empty());
}

TEST(UniversalSDKTest, GetSDKInfoContainsVersion) {
    SDK sdk;
    EXPECT_NE(sdk.GetSDKInfo().find(sdk.GetVersion()), std::string::npos);
}

// ---------------------------------------------------------------------------
// Feature list
// ---------------------------------------------------------------------------

TEST(UniversalSDKTest, GetAvailableFeaturesIsNonEmpty) {
    SDK sdk;
    EXPECT_FALSE(sdk.GetAvailableFeatures().empty());
}

// ---------------------------------------------------------------------------
// System info and command execution
// ---------------------------------------------------------------------------

TEST(UniversalSDKTest, GetSystemInfoReturnsNotInitializedWhenNotReady) {
    SDK sdk;
    EXPECT_NE(sdk.GetSystemInfo().find("not initialized"), std::string::npos);
}

TEST(UniversalSDKTest, GetSystemInfoDelegatesToPlatformImplWhenInitialized) {
    auto sdk = MakeInitialized();
    const std::string info = sdk->GetSystemInfo();
    EXPECT_FALSE(info.empty());
    EXPECT_EQ(info.find("not initialized"), std::string::npos);
}

TEST(UniversalSDKTest, ExecuteCommandReturnsFalseWhenNotInitialized) {
    SDK sdk;
    EXPECT_FALSE(sdk.ExecuteCommand("echo hi"));
}

// ---------------------------------------------------------------------------
// CreateLegalAssistant factory
// ---------------------------------------------------------------------------

TEST(UniversalSDKTest, CreateLegalAssistantReturnsNonNull) {
    auto sdk = MakeInitialized();

    auto connector = std::make_shared<InMemoryInmateDataConnector>();
    connector->AddInmate("I-001", "Pending appeal.");
    connector->GrantAccess("I-001", AccessRole::LEGAL_ADVOCATE);

    auto assistant = sdk->CreateLegalAssistant(
        std::make_shared<EchoLLMProvider>(),
        std::make_shared<InMemoryRetrievalProvider>(
            std::vector<Citation>{{"src", "Title", "Ref"}}),
        connector,
        std::make_shared<DefaultPolicyEngine>());

    EXPECT_NE(assistant, nullptr);
}

TEST(UniversalSDKTest, CreateLegalAssistantProducesWorkingAssistant) {
    auto sdk = MakeInitialized();

    auto connector = std::make_shared<InMemoryInmateDataConnector>();
    connector->AddInmate("I-001", "Pending appeal.");
    connector->GrantAccess("I-001", AccessRole::LEGAL_ADVOCATE);

    LegalAssistantConfig config;
    config.allowed_jurisdictions = {"CA"};
    config.allowed_case_types = {"appeal"};

    auto assistant = sdk->CreateLegalAssistant(
        std::make_shared<EchoLLMProvider>(),
        std::make_shared<InMemoryRetrievalProvider>(
            std::vector<Citation>{{"src", "Title", "Ref"}}),
        connector,
        std::make_shared<DefaultPolicyEngine>(),
        config);

    LegalQueryContext ctx;
    ctx.inmate_id = "I-001";
    ctx.jurisdiction = "CA";
    ctx.case_type = "appeal";
    ctx.question = "What are my rights?";
    ctx.role = AccessRole::LEGAL_ADVOCATE;
    ctx.consent_granted = true;

    const LegalAdviceResponse resp = assistant->Advise(ctx);
    EXPECT_FALSE(resp.escalated);
    EXPECT_FALSE(resp.summary.empty());
}
