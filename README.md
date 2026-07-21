# universal-sdk

Universal cross platform development SDK with optional legal-assistance interfaces.

## Core features
- Cross-platform initialization and platform detection (Windows, Linux, Android)
- System information and command execution via platform-specific implementations
- Extensible legal-assistant module interfaces for:
  - LLM advice generation
  - Legal citation retrieval
  - Inmate data access connectors
  - Policy-based escalation and audit logging
- Bundled stub providers for local development and testing
- Optional persistent audit log (`FileAuditWriter`)

## Legal-assistant guardrails
- Configurable jurisdiction and case-type coverage
- Consent and role-based access checks
- INMATE role enforcement: an inmate requester may only query their own records
- Confidence-threshold enforcement
- Mandatory disclaimers and human-escalation paths
- In-memory audit trail + optional external audit writer for traceability

---

## Building

### Prerequisites
- CMake ≥ 3.10
- C++17 compiler (GCC, Clang, or MSVC)
- (optional) Google Test for running the test suite

### Quick build
```bash
bash build.sh
```

Options:
| Flag | Description |
|------|-------------|
| `-d` / `--debug` | Build in Debug mode (default: Release) |
| `-b DIR` | Set the build directory (default: `build`) |
| `-c` | Clean the build directory first |
| `-i` | Install after building |

### Manual CMake build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Running tests
Install Google Test first (Ubuntu):
```bash
sudo apt-get install -y libgtest-dev
cd /usr/src/googletest && sudo cmake . && sudo cmake --build . --target install
```

Then build and test:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

---

## Quickstart

### 1. Include the headers
```cpp
#include "universal_sdk.h"
#include "stub_providers.h"    // EchoLLMProvider, InMemoryRetrievalProvider, etc.
#include "file_audit_writer.h" // optional persistent audit log
```

### 2. Wire up providers
```cpp
using namespace UniversalSDK;

// Inmate data store
auto connector = std::make_shared<InMemoryInmateDataConnector>();
connector->AddInmate("I-001", "Pending appeal filed 2026-03.");
connector->GrantAccess("I-001", AccessRole::LEGAL_ADVOCATE);

// LLM and retrieval (replace with real implementations for production)
auto llm       = std::make_shared<EchoLLMProvider>(/*confidence=*/0.9);
auto retrieval = std::make_shared<InMemoryRetrievalProvider>(
    std::vector<Citation>{{"CA Penal Code § 1237", "Grounds for appeal", "PC-1237"}});

// Coverage policy
LegalAssistantConfig config;
config.allowed_jurisdictions = {"CA"};
config.allowed_case_types    = {"appeal"};
```

### 3. Attach an audit writer (optional)
```cpp
auto writer = std::make_shared<FileAuditWriter>("/var/log/legal_audit.ndjson");
```

### 4. Create the assistant and query
```cpp
LegalAssistant assistant(
    llm, retrieval, connector,
    std::make_shared<DefaultPolicyEngine>(),
    config,
    writer);  // omit writer to use in-memory audit trail only

LegalQueryContext ctx;
ctx.inmate_id       = "I-001";
ctx.jurisdiction    = "CA";
ctx.case_type       = "appeal";
ctx.question        = "What grounds support a criminal appeal?";
ctx.role            = AccessRole::LEGAL_ADVOCATE;
ctx.consent_granted = true;

LegalAdviceResponse resp = assistant.Advise(ctx);
if (!resp.escalated) {
    std::cout << resp.summary << "\n";
}
```

### Full example
See [`examples/legal_assistant_sample.cpp`](examples/legal_assistant_sample.cpp) for a
complete runnable demonstration.

Build and run the sample:
```bash
cmake -B build && cmake --build build --target legal_assistant_sample
./build/legal_assistant_sample
```

---

## Implementing your own providers

| Interface | Purpose |
|-----------|---------|
| `LLMProvider` | Generate advice text and estimate confidence |
| `RetrievalProvider` | Return `Citation` objects for a jurisdiction/case-type/question |
| `InmateDataConnector` | Gate access and supply case summaries |
| `PolicyEngine` | Final allow/deny/escalate decision |
| `AuditWriter` | Receive `AuditEvent` records for external persistence |

Stub implementations (`EchoLLMProvider`, `InMemoryRetrievalProvider`,
`InMemoryInmateDataConnector`) are included in `include/stub_providers.h` and
`src/stub_providers.cpp` for testing and local development.

