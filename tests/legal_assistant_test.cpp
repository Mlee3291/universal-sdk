#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "legal_assistant.h"

using namespace UniversalSDK;

// ---------------------------------------------------------------------------
// Test doubles
// ---------------------------------------------------------------------------

class FakeLLMProvider : public LLMProvider {
public:
    explicit FakeLLMProvider(double confidence = 0.9)
        : confidence_(confidence) {}

    std::string GenerateAdvice(const std::string& /*prompt*/) override {
        return "Fake legal guidance.";
    }

    double EstimateConfidence(const std::string& /*generated_text*/) override {
        return confidence_;
    }

private:
    double confidence_;
};

class FakeRetrievalProvider : public RetrievalProvider {
public:
    explicit FakeRetrievalProvider(bool return_citations = true)
        : return_citations_(return_citations) {}

    std::vector<Citation> RetrieveCitations(
        const std::string& /*jurisdiction*/,
        const std::string& /*case_type*/,
        const std::string& /*question*/) override {
        if (return_citations_) {
            return {{"source1", "Title A", "Ref 1"}};
        }
        return {};
    }

private:
    bool return_citations_;
};

class FakeInmateDataConnector : public InmateDataConnector {
public:
    explicit FakeInmateDataConnector(bool has_access = true)
        : has_access_(has_access) {}

    bool HasAccessToInmate(const std::string& /*inmate_id*/, AccessRole /*role*/) override {
        return has_access_;
    }

    std::string GetInmateCaseSummary(const std::string& /*inmate_id*/) override {
        return "Inmate has a pending appeal case.";
    }

private:
    bool has_access_;
};

// ---------------------------------------------------------------------------
// Helper: build a fully-configured LegalAssistant with sensible defaults.
// ---------------------------------------------------------------------------

static LegalQueryContext MakeValidContext() {
    LegalQueryContext ctx;
    ctx.inmate_id = "I-001";
    ctx.jurisdiction = "CA";
    ctx.case_type = "appeal";
    ctx.question = "What are my appeal rights?";
    ctx.role = AccessRole::LEGAL_ADVOCATE;
    ctx.consent_granted = true;
    return ctx;
}

static LegalAssistantConfig MakeConfig() {
    LegalAssistantConfig cfg;
    cfg.allowed_jurisdictions = {"CA", "NY"};
    cfg.allowed_case_types = {"appeal", "parole"};
    return cfg;
}

static std::shared_ptr<LegalAssistant> MakeAssistant(
    double llm_confidence = 0.9,
    bool has_access = true,
    bool return_citations = true,
    LegalAssistantConfig config = MakeConfig()) {
    return std::make_shared<LegalAssistant>(
        std::make_shared<FakeLLMProvider>(llm_confidence),
        std::make_shared<FakeRetrievalProvider>(return_citations),
        std::make_shared<FakeInmateDataConnector>(has_access),
        std::make_shared<DefaultPolicyEngine>(),
        std::move(config));
}

// ---------------------------------------------------------------------------
// DefaultPolicyEngine – individual policy gate tests
// ---------------------------------------------------------------------------

class PolicyEngineTest : public ::testing::Test {
protected:
    DefaultPolicyEngine engine_;
    LegalAssistantConfig config_ = MakeConfig();
    std::vector<Citation> citations_{{{"source", "Title", "Ref"}}};
};

TEST_F(PolicyEngineTest, AllowsWhenAllCheckPass) {
    LegalQueryContext ctx = MakeValidContext();
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, config_);
    EXPECT_TRUE(d.allow_response);
    EXPECT_FALSE(d.escalate_to_human);
}

TEST_F(PolicyEngineTest, DeniesWhenConsentMissing) {
    LegalQueryContext ctx = MakeValidContext();
    ctx.consent_granted = false;
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, config_);
    EXPECT_FALSE(d.allow_response);
    EXPECT_NE(d.reason.find("consent"), std::string::npos);
}

TEST_F(PolicyEngineTest, DeniesWhenJurisdictionNotInAllowedList) {
    LegalQueryContext ctx = MakeValidContext();
    ctx.jurisdiction = "TX";  // not in {"CA","NY"}
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, config_);
    EXPECT_FALSE(d.allow_response);
    EXPECT_NE(d.reason.find("Jurisdiction"), std::string::npos);
}

TEST_F(PolicyEngineTest, DeniesWhenAllowedJurisdictionsListIsEmpty) {
    // Empty allowed list must deny all (fixed from return true bug).
    LegalAssistantConfig cfg;  // empty allowed_jurisdictions
    cfg.allowed_case_types = {"appeal"};
    LegalQueryContext ctx = MakeValidContext();
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, cfg);
    EXPECT_FALSE(d.allow_response);
}

TEST_F(PolicyEngineTest, DeniesWhenCaseTypeNotInAllowedList) {
    LegalQueryContext ctx = MakeValidContext();
    ctx.case_type = "civil";  // not in {"appeal","parole"}
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, config_);
    EXPECT_FALSE(d.allow_response);
    EXPECT_NE(d.reason.find("Case type"), std::string::npos);
}

TEST_F(PolicyEngineTest, DeniesWhenAllowedCaseTypesListIsEmpty) {
    // Empty allowed list must deny all (fixed from return true bug).
    LegalAssistantConfig cfg;
    cfg.allowed_jurisdictions = {"CA"};  // empty allowed_case_types
    LegalQueryContext ctx = MakeValidContext();
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, cfg);
    EXPECT_FALSE(d.allow_response);
}

TEST_F(PolicyEngineTest, DeniesWhenRoleIsUnknown) {
    LegalQueryContext ctx = MakeValidContext();
    ctx.role = AccessRole::UNKNOWN;
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, config_);
    EXPECT_FALSE(d.allow_response);
    EXPECT_NE(d.reason.find("role"), std::string::npos);
}

TEST_F(PolicyEngineTest, DeniesWhenConfidenceBelowThreshold) {
    LegalQueryContext ctx = MakeValidContext();
    PolicyDecision d = engine_.Evaluate(ctx, 0.5, citations_, config_);  // threshold is 0.75
    EXPECT_FALSE(d.allow_response);
    EXPECT_NE(d.reason.find("confidence"), std::string::npos);
}

TEST_F(PolicyEngineTest, DeniesWhenNoCitations) {
    LegalQueryContext ctx = MakeValidContext();
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, {}, config_);
    EXPECT_FALSE(d.allow_response);
    EXPECT_NE(d.reason.find("citation"), std::string::npos);
}

TEST_F(PolicyEngineTest, AlwaysIncludesDisclaimers) {
    LegalQueryContext ctx = MakeValidContext();
    PolicyDecision d = engine_.Evaluate(ctx, 0.9, citations_, config_);
    EXPECT_FALSE(d.disclaimers.empty());
}

// ---------------------------------------------------------------------------
// LegalAssistant constructor – missing dependency validation
// ---------------------------------------------------------------------------

TEST(LegalAssistantConstructorTest, ThrowsWhenLLMProviderIsNull) {
    EXPECT_THROW(
        LegalAssistant(nullptr,
                       std::make_shared<FakeRetrievalProvider>(),
                       std::make_shared<FakeInmateDataConnector>(),
                       std::make_shared<DefaultPolicyEngine>()),
        std::invalid_argument);
}

TEST(LegalAssistantConstructorTest, ThrowsWhenRetrievalProviderIsNull) {
    EXPECT_THROW(
        LegalAssistant(std::make_shared<FakeLLMProvider>(),
                       nullptr,
                       std::make_shared<FakeInmateDataConnector>(),
                       std::make_shared<DefaultPolicyEngine>()),
        std::invalid_argument);
}

TEST(LegalAssistantConstructorTest, ThrowsWhenDataConnectorIsNull) {
    EXPECT_THROW(
        LegalAssistant(std::make_shared<FakeLLMProvider>(),
                       std::make_shared<FakeRetrievalProvider>(),
                       nullptr,
                       std::make_shared<DefaultPolicyEngine>()),
        std::invalid_argument);
}

TEST(LegalAssistantConstructorTest, ThrowsWhenPolicyEngineIsNull) {
    EXPECT_THROW(
        LegalAssistant(std::make_shared<FakeLLMProvider>(),
                       std::make_shared<FakeRetrievalProvider>(),
                       std::make_shared<FakeInmateDataConnector>(),
                       nullptr),
        std::invalid_argument);
}

TEST(LegalAssistantConstructorTest, ErrorMessageNamesTheMissingDependency) {
    try {
        LegalAssistant(nullptr,
                       std::make_shared<FakeRetrievalProvider>(),
                       std::make_shared<FakeInmateDataConnector>(),
                       std::make_shared<DefaultPolicyEngine>());
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_NE(std::string(ex.what()).find("llm_provider"), std::string::npos);
    }
}

TEST(LegalAssistantConstructorTest, SucceedsWithAllProvidersSet) {
    EXPECT_NO_THROW(MakeAssistant());
}

// ---------------------------------------------------------------------------
// LegalAssistant::Advise – high-level flow tests
// ---------------------------------------------------------------------------

TEST(LegalAssistantAdviseTest, AccessDeniedWhenNoInmateAccess) {
    auto assistant = MakeAssistant(0.9, /*has_access=*/false);
    LegalAdviceResponse resp = assistant->Advise(MakeValidContext());
    EXPECT_TRUE(resp.escalated);
    EXPECT_NE(resp.escalation_reason.find("permission"), std::string::npos);
}

TEST(LegalAssistantAdviseTest, EscalatesWhenPolicyDenies) {
    // Low confidence triggers escalation.
    auto assistant = MakeAssistant(/*llm_confidence=*/0.3);
    LegalAdviceResponse resp = assistant->Advise(MakeValidContext());
    EXPECT_TRUE(resp.escalated);
    EXPECT_FALSE(resp.guidance_steps.empty());
}

TEST(LegalAssistantAdviseTest, GeneratesAdviceWhenPolicyPasses) {
    auto assistant = MakeAssistant();
    LegalAdviceResponse resp = assistant->Advise(MakeValidContext());
    EXPECT_FALSE(resp.escalated);
    EXPECT_FALSE(resp.summary.empty());
    EXPECT_FALSE(resp.citations.empty());
    EXPECT_FALSE(resp.guidance_steps.empty());
}

TEST(LegalAssistantAdviseTest, ResponseIncludesDisclaimers) {
    auto assistant = MakeAssistant();
    LegalAdviceResponse resp = assistant->Advise(MakeValidContext());
    EXPECT_FALSE(resp.disclaimers.empty());
}

// ---------------------------------------------------------------------------
// Audit trail
// ---------------------------------------------------------------------------

TEST(AuditTrailTest, AccessDeniedRecordsAccessDeniedAction) {
    auto assistant = MakeAssistant(0.9, /*has_access=*/false);
    assistant->Advise(MakeValidContext());
    const auto& trail = assistant->GetAuditTrail();
    ASSERT_EQ(trail.size(), 1u);
    EXPECT_EQ(trail[0].action, "access_denied");
}

TEST(AuditTrailTest, EscalationRecordsEscalatedAction) {
    auto assistant = MakeAssistant(/*llm_confidence=*/0.3);
    assistant->Advise(MakeValidContext());
    const auto& trail = assistant->GetAuditTrail();
    ASSERT_EQ(trail.size(), 1u);
    EXPECT_EQ(trail[0].action, "escalated");
}

TEST(AuditTrailTest, SuccessfulAdviceRecordsAdviceGeneratedAction) {
    auto assistant = MakeAssistant();
    assistant->Advise(MakeValidContext());
    const auto& trail = assistant->GetAuditTrail();
    ASSERT_EQ(trail.size(), 1u);
    EXPECT_EQ(trail[0].action, "advice_generated");
}

TEST(AuditTrailTest, AuditEventHasTimestampAndAuditId) {
    auto assistant = MakeAssistant();
    assistant->Advise(MakeValidContext());
    const auto& event = assistant->GetAuditTrail()[0];
    EXPECT_FALSE(event.timestamp_utc.empty());
    EXPECT_FALSE(event.audit_id.empty());
}

TEST(AuditTrailTest, MultipleCallsAccumulateAuditEvents) {
    auto assistant = MakeAssistant();
    assistant->Advise(MakeValidContext());
    assistant->Advise(MakeValidContext());
    EXPECT_EQ(assistant->GetAuditTrail().size(), 2u);
}

// ---------------------------------------------------------------------------
// NextAuditId – sequential uniqueness
// ---------------------------------------------------------------------------

TEST(AuditIdTest, AuditIdsAreUniqueAcrossCalls) {
    auto assistant = MakeAssistant();
    for (int i = 0; i < 5; ++i) {
        assistant->Advise(MakeValidContext());
    }
    const auto& trail = assistant->GetAuditTrail();
    std::unordered_set<std::string> ids;
    for (const auto& event : trail) {
        ids.insert(event.audit_id);
    }
    EXPECT_EQ(ids.size(), trail.size());
}

TEST(AuditIdTest, AuditIdsAreUniqueUnderConcurrentAdviceCalls) {
    auto assistant = MakeAssistant();
    constexpr int kThreads = 8;
    constexpr int kCallsPerThread = 10;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kCallsPerThread; ++i) {
                assistant->Advise(MakeValidContext());
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    const auto& trail = assistant->GetAuditTrail();
    std::unordered_set<std::string> ids;
    for (const auto& event : trail) {
        ids.insert(event.audit_id);
    }
    EXPECT_EQ(ids.size(), trail.size());
}
