#ifndef STUB_PROVIDERS_H
#define STUB_PROVIDERS_H

#include "legal_assistant.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace UniversalSDK {

// ---------------------------------------------------------------------------
// EchoLLMProvider
// Returns the prompt text as the generated advice with a fixed confidence.
// Useful for local development and integration testing without a real LLM.
// ---------------------------------------------------------------------------
class EchoLLMProvider : public LLMProvider {
public:
    explicit EchoLLMProvider(double confidence = 1.0);

    std::string GenerateAdvice(const std::string& prompt) override;
    double EstimateConfidence(const std::string& generated_text) override;

private:
    double confidence_;
};

// ---------------------------------------------------------------------------
// InMemoryRetrievalProvider
// Returns a pre-loaded list of Citations regardless of the query parameters.
// Useful for integration tests and sample programs.
// ---------------------------------------------------------------------------
class InMemoryRetrievalProvider : public RetrievalProvider {
public:
    explicit InMemoryRetrievalProvider(std::vector<Citation> citations = {});

    std::vector<Citation> RetrieveCitations(
        const std::string& jurisdiction,
        const std::string& case_type,
        const std::string& question) override;

private:
    std::vector<Citation> citations_;
};

// ---------------------------------------------------------------------------
// InMemoryInmateDataConnector
// Stores inmate case summaries and role-based access grants in memory.
// Records are added via AddInmate() and GrantAccess() before use.
// ---------------------------------------------------------------------------
class InMemoryInmateDataConnector : public InmateDataConnector {
public:
    // Register an inmate and their case summary.
    void AddInmate(const std::string& inmate_id, const std::string& case_summary);

    // Grant the given role access to the specified inmate record.
    void GrantAccess(const std::string& inmate_id, AccessRole role);

    bool HasAccessToInmate(const std::string& inmate_id, AccessRole role) override;
    std::string GetInmateCaseSummary(const std::string& inmate_id) override;

private:
    std::unordered_map<std::string, std::string> summaries_;
    std::unordered_map<std::string, std::vector<AccessRole>> access_map_;
};

} // namespace UniversalSDK

#endif // STUB_PROVIDERS_H
