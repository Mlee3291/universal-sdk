#ifndef LEGAL_ASSISTANT_H
#define LEGAL_ASSISTANT_H

#include <memory>
#include <string>
#include <vector>
#include <atomic>

namespace UniversalSDK {

constexpr double kDefaultConfidenceThreshold = 0.75;

enum class AccessRole {
    INMATE,
    FACILITY_STAFF,
    LEGAL_ADVOCATE,
    UNKNOWN
};

struct Citation {
    std::string source;
    std::string title;
    std::string reference;
};

struct LegalQueryContext {
    std::string inmate_id;
    std::string jurisdiction;
    std::string case_type;
    std::string question;
    AccessRole role = AccessRole::UNKNOWN;
    bool consent_granted = false;
};

struct PolicyDecision {
    bool allow_response = false;
    bool escalate_to_human = true;
    std::string reason;
    std::vector<std::string> disclaimers;
};

struct LegalAdviceResponse {
    std::string summary;
    std::vector<std::string> guidance_steps;
    std::vector<std::string> disclaimers;
    std::vector<Citation> citations;
    double confidence = 0.0;
    bool escalated = true;
    std::string escalation_reason;
    std::string audit_id;
};

struct AuditEvent {
    std::string audit_id;
    std::string timestamp_utc;
    std::string action;
    std::string details;
};

struct LegalAssistantConfig {
    std::vector<std::string> allowed_jurisdictions;
    std::vector<std::string> allowed_case_types;
    double confidence_threshold = kDefaultConfidenceThreshold;
};

class LLMProvider {
public:
    virtual ~LLMProvider() = default;
    virtual std::string GenerateAdvice(const std::string& prompt) = 0;
    virtual double EstimateConfidence(const std::string& generated_text) = 0;
};

class RetrievalProvider {
public:
    virtual ~RetrievalProvider() = default;
    virtual std::vector<Citation> RetrieveCitations(
        const std::string& jurisdiction,
        const std::string& case_type,
        const std::string& question) = 0;
};

class InmateDataConnector {
public:
    virtual ~InmateDataConnector() = default;
    virtual bool HasAccessToInmate(const std::string& inmate_id, AccessRole role) = 0;
    virtual std::string GetInmateCaseSummary(const std::string& inmate_id) = 0;
};

class PolicyEngine {
public:
    virtual ~PolicyEngine() = default;
    virtual PolicyDecision Evaluate(
        const LegalQueryContext& context,
        double confidence,
        const std::vector<Citation>& citations,
        const LegalAssistantConfig& config) = 0;
};

class DefaultPolicyEngine : public PolicyEngine {
public:
    PolicyDecision Evaluate(
        const LegalQueryContext& context,
        double confidence,
        const std::vector<Citation>& citations,
        const LegalAssistantConfig& config) override;
};

class LegalAssistant {
public:
    LegalAssistant(
        std::shared_ptr<LLMProvider> llm_provider,
        std::shared_ptr<RetrievalProvider> retrieval_provider,
        std::shared_ptr<InmateDataConnector> data_connector,
        std::shared_ptr<PolicyEngine> policy_engine,
        LegalAssistantConfig config = LegalAssistantConfig{});

    LegalAdviceResponse Advise(const LegalQueryContext& context);
    const std::vector<AuditEvent>& GetAuditTrail() const;

private:
    std::string BuildPrompt(const LegalQueryContext& context, const std::string& inmate_summary) const;
    std::string NextAuditId();
    void AppendAudit(const std::string& action, const std::string& details, const std::string& audit_id);

    std::shared_ptr<LLMProvider> llm_provider_;
    std::shared_ptr<RetrievalProvider> retrieval_provider_;
    std::shared_ptr<InmateDataConnector> data_connector_;
    std::shared_ptr<PolicyEngine> policy_engine_;
    LegalAssistantConfig config_;
    std::vector<AuditEvent> audit_trail_;
    std::atomic<unsigned long long> audit_sequence_;
};

} // namespace UniversalSDK

#endif // LEGAL_ASSISTANT_H
