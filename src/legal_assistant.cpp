#include "legal_assistant.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace UniversalSDK {

namespace {

constexpr const char* kActionAccessDenied = "access_denied";
constexpr const char* kActionEscalated = "escalated";
constexpr const char* kActionAdviceGenerated = "advice_generated";

std::string ToIsoTimestampUtc() {
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &time_t_now);
#else
    gmtime_r(&time_t_now, &tm);
#endif

    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

bool ContainsValue(const std::vector<std::string>& values, const std::string& target) {
    if (values.empty()) {
        return false;
    }

    return std::find(values.begin(), values.end(), target) != values.end();
}

} // namespace

PolicyDecision DefaultPolicyEngine::Evaluate(
    const LegalQueryContext& context,
    double confidence,
    const std::vector<Citation>& citations,
    const LegalAssistantConfig& config) {
    PolicyDecision decision;
    decision.disclaimers = {
        "This tool provides legal information and is not a licensed attorney.",
        "For high-risk or urgent matters, request review from a licensed attorney or legal aid organization."
    };

    if (!context.consent_granted) {
        decision.reason = "Missing inmate consent for legal-data processing.";
        return decision;
    }

    if (!ContainsValue(config.allowed_jurisdictions, context.jurisdiction)) {
        decision.reason = "Jurisdiction is outside configured legal coverage.";
        return decision;
    }

    if (!ContainsValue(config.allowed_case_types, context.case_type)) {
        decision.reason = "Case type is outside configured legal coverage.";
        return decision;
    }

    if (context.role == AccessRole::UNKNOWN) {
        decision.reason = "Unknown requester role.";
        return decision;
    }

    if (confidence < config.confidence_threshold) {
        decision.reason = "Model confidence is below policy threshold.";
        return decision;
    }

    if (citations.empty()) {
        decision.reason = "No legal citations available for verification.";
        return decision;
    }

    decision.allow_response = true;
    decision.escalate_to_human = false;
    decision.reason = "Policy checks passed.";
    return decision;
}

LegalAssistant::LegalAssistant(
    std::shared_ptr<LLMProvider> llm_provider,
    std::shared_ptr<RetrievalProvider> retrieval_provider,
    std::shared_ptr<InmateDataConnector> data_connector,
    std::shared_ptr<PolicyEngine> policy_engine,
    LegalAssistantConfig config)
    : llm_provider_(std::move(llm_provider)),
      retrieval_provider_(std::move(retrieval_provider)),
      data_connector_(std::move(data_connector)),
      policy_engine_(std::move(policy_engine)),
      config_(std::move(config)),
      audit_sequence_(0) {
    if (!llm_provider_ || !retrieval_provider_ || !data_connector_ || !policy_engine_) {
        std::string missing;
        if (!llm_provider_) {
            missing += "llm_provider ";
        }
        if (!retrieval_provider_) {
            missing += "retrieval_provider ";
        }
        if (!data_connector_) {
            missing += "data_connector ";
        }
        if (!policy_engine_) {
            missing += "policy_engine ";
        }
        throw std::invalid_argument("Missing required dependency: " + missing);
    }
}

LegalAdviceResponse LegalAssistant::Advise(const LegalQueryContext& context) {
    LegalAdviceResponse response;
    response.audit_id = NextAuditId();

    if (!data_connector_->HasAccessToInmate(context.inmate_id, context.role)) {
        response.escalation_reason = "Requester does not have permission to access inmate records.";
        AppendAudit(kActionAccessDenied, response.escalation_reason, response.audit_id);
        return response;
    }

    const std::string inmate_summary = data_connector_->GetInmateCaseSummary(context.inmate_id);
    const std::vector<Citation> citations = retrieval_provider_->RetrieveCitations(
        context.jurisdiction,
        context.case_type,
        context.question);

    const std::string prompt = BuildPrompt(context, inmate_summary);
    response.summary = llm_provider_->GenerateAdvice(prompt);
    response.confidence = llm_provider_->EstimateConfidence(response.summary);
    response.citations = citations;

    const PolicyDecision decision = policy_engine_->Evaluate(context, response.confidence, citations, config_);
    response.disclaimers = decision.disclaimers;

    if (!decision.allow_response) {
        response.summary = "Automatic legal guidance is restricted by policy checks.";
        response.escalated = true;
        response.escalation_reason = decision.reason;
        response.guidance_steps = {
            "Request review by a licensed attorney or legal aid partner.",
            "Confirm jurisdiction and case type coverage for this request.",
            "Verify inmate consent and record-sharing authorization."
        };
        AppendAudit(kActionEscalated, decision.reason, response.audit_id);
        return response;
    }

    response.escalated = decision.escalate_to_human;
    response.escalation_reason = decision.reason;
    response.guidance_steps = {
        "Review cited legal sources before filing or acting.",
        "Verify all filing deadlines with facility legal resources.",
        "Escalate to legal counsel if facts change or risk increases."
    };

    AppendAudit(kActionAdviceGenerated, "Policy-approved response produced with citations.", response.audit_id);
    return response;
}

const std::vector<AuditEvent>& LegalAssistant::GetAuditTrail() const {
    std::lock_guard<std::mutex> lock(audit_trail_mutex_);
    return audit_trail_;
}

std::string LegalAssistant::BuildPrompt(
    const LegalQueryContext& context,
    const std::string& inmate_summary) const {
    std::ostringstream prompt;
    prompt << "You are an inmate legal assistance system providing informational guidance only.\n";
    prompt << "Jurisdiction: " << context.jurisdiction << "\n";
    prompt << "Case type: " << context.case_type << "\n";
    prompt << "Inmate summary: " << inmate_summary << "\n";
    prompt << "Question: " << context.question << "\n";
    prompt << "Respond with plain-language next steps, rights considerations, and filing cautions.\n";
    return prompt.str();
}

std::string LegalAssistant::NextAuditId() {
    const unsigned long long sequence = audit_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
    std::ostringstream stream;
    stream << "audit-" << sequence;
    return stream.str();
}

void LegalAssistant::AppendAudit(
    const std::string& action,
    const std::string& details,
    const std::string& audit_id) {
    std::lock_guard<std::mutex> lock(audit_trail_mutex_);
    audit_trail_.push_back({
        audit_id,
        ToIsoTimestampUtc(),
        action,
        details
    });
}

} // namespace UniversalSDK
