// legal_assistant_sample.cpp
//
// Minimal end-to-end sample showing how to wire up and use the
// UniversalSDK legal-assistant module with the bundled stub providers.
//
// Build:
//   cmake -B build && cmake --build build --target legal_assistant_sample
// Run:
//   ./build/legal_assistant_sample

#include <iostream>
#include <memory>
#include <vector>

#include "file_audit_writer.h"
#include "legal_assistant.h"
#include "stub_providers.h"

int main() {
    using namespace UniversalSDK;

    // -----------------------------------------------------------------------
    // 1.  Configure an InMemoryInmateDataConnector with one inmate record.
    // -----------------------------------------------------------------------
    auto connector = std::make_shared<InMemoryInmateDataConnector>();
    connector->AddInmate("I-001", "Defendant has a pending felony-appeal filed 2026-03.");
    connector->GrantAccess("I-001", AccessRole::LEGAL_ADVOCATE);
    connector->GrantAccess("I-001", AccessRole::INMATE);

    // -----------------------------------------------------------------------
    // 2.  Choose an LLMProvider.  EchoLLMProvider echoes the prompt back so
    //     the sample runs without any real model or network connection.
    // -----------------------------------------------------------------------
    auto llm = std::make_shared<EchoLLMProvider>(/*confidence=*/0.9);

    // -----------------------------------------------------------------------
    // 3.  Pre-load citations for the retrieval step.
    // -----------------------------------------------------------------------
    std::vector<Citation> citations = {
        {"CA Penal Code § 1237", "Grounds for appeal", "PC-1237"},
        {"People v. Smith (2020)", "Ineffective assistance standard", "CA-APP-2020"},
    };
    auto retrieval = std::make_shared<InMemoryRetrievalProvider>(citations);

    // -----------------------------------------------------------------------
    // 4.  Restrict coverage to California / appeal queries.
    // -----------------------------------------------------------------------
    LegalAssistantConfig config;
    config.allowed_jurisdictions = {"CA"};
    config.allowed_case_types    = {"appeal"};
    config.confidence_threshold  = 0.75;

    // -----------------------------------------------------------------------
    // 5.  Attach a FileAuditWriter so every query is persisted to disk.
    // -----------------------------------------------------------------------
    const std::string audit_log_path = "/tmp/legal_audit.ndjson";
    std::shared_ptr<AuditWriter> writer;
    try {
        writer = std::make_shared<FileAuditWriter>(audit_log_path);
        std::cout << "Audit log: " << audit_log_path << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Warning: could not open audit log – " << ex.what() << "\n";
    }

    // -----------------------------------------------------------------------
    // 6.  Construct the LegalAssistant.
    // -----------------------------------------------------------------------
    LegalAssistant assistant(
        llm,
        retrieval,
        connector,
        std::make_shared<DefaultPolicyEngine>(),
        config,
        writer);

    // -----------------------------------------------------------------------
    // 7.  Submit a query on behalf of a legal advocate.
    // -----------------------------------------------------------------------
    LegalQueryContext ctx;
    ctx.inmate_id      = "I-001";
    ctx.jurisdiction   = "CA";
    ctx.case_type      = "appeal";
    ctx.question       = "What are the grounds for filing a criminal appeal in California?";
    ctx.role           = AccessRole::LEGAL_ADVOCATE;
    ctx.consent_granted = true;

    LegalAdviceResponse resp = assistant.Advise(ctx);

    // -----------------------------------------------------------------------
    // 8.  Print the response.
    // -----------------------------------------------------------------------
    std::cout << "\n=== Legal Advice Response ===\n";
    std::cout << "Audit ID   : " << resp.audit_id       << "\n";
    std::cout << "Escalated  : " << (resp.escalated ? "yes" : "no") << "\n";
    if (resp.escalated) {
        std::cout << "Reason     : " << resp.escalation_reason << "\n";
    }
    std::cout << "Summary    : " << resp.summary << "\n";

    std::cout << "\nGuidance steps:\n";
    for (const auto& step : resp.guidance_steps) {
        std::cout << "  - " << step << "\n";
    }

    std::cout << "\nCitations:\n";
    for (const auto& c : resp.citations) {
        std::cout << "  [" << c.reference << "] " << c.title << " (" << c.source << ")\n";
    }

    std::cout << "\nDisclaimers:\n";
    for (const auto& d : resp.disclaimers) {
        std::cout << "  * " << d << "\n";
    }

    // -----------------------------------------------------------------------
    // 9.  Show the in-memory audit trail.
    // -----------------------------------------------------------------------
    std::cout << "\nAudit trail (" << assistant.GetAuditTrail().size() << " event(s)):\n";
    for (const auto& ev : assistant.GetAuditTrail()) {
        std::cout << "  [" << ev.timestamp_utc << "] "
                  << ev.audit_id << " – " << ev.action << ": " << ev.details << "\n";
    }

    return 0;
}
