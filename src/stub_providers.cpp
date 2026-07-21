#include "stub_providers.h"

#include <sstream>

namespace UniversalSDK {

// ---------------------------------------------------------------------------
// EchoLLMProvider
// ---------------------------------------------------------------------------

EchoLLMProvider::EchoLLMProvider(double confidence)
    : confidence_(confidence) {}

std::string EchoLLMProvider::GenerateAdvice(const std::string& prompt) {
    return "[EchoLLMProvider] " + prompt;
}

double EchoLLMProvider::EstimateConfidence(const std::string& /*generated_text*/) {
    return confidence_;
}

// ---------------------------------------------------------------------------
// InMemoryRetrievalProvider
// ---------------------------------------------------------------------------

InMemoryRetrievalProvider::InMemoryRetrievalProvider(std::vector<Citation> citations)
    : citations_(std::move(citations)) {}

std::vector<Citation> InMemoryRetrievalProvider::RetrieveCitations(
    const std::string& /*jurisdiction*/,
    const std::string& /*case_type*/,
    const std::string& /*question*/) {
    return citations_;
}

// ---------------------------------------------------------------------------
// InMemoryInmateDataConnector
// ---------------------------------------------------------------------------

void InMemoryInmateDataConnector::AddInmate(
    const std::string& inmate_id,
    const std::string& case_summary) {
    summaries_[inmate_id] = case_summary;
}

void InMemoryInmateDataConnector::GrantAccess(
    const std::string& inmate_id,
    AccessRole role) {
    access_map_[inmate_id].push_back(role);
}

bool InMemoryInmateDataConnector::HasAccessToInmate(
    const std::string& inmate_id,
    AccessRole role) {
    auto it = access_map_.find(inmate_id);
    if (it == access_map_.end()) {
        return false;
    }
    const auto& roles = it->second;
    return std::find(roles.begin(), roles.end(), role) != roles.end();
}

std::string InMemoryInmateDataConnector::GetInmateCaseSummary(
    const std::string& inmate_id) {
    auto it = summaries_.find(inmate_id);
    if (it == summaries_.end()) {
        return "";
    }
    return it->second;
}

} // namespace UniversalSDK
