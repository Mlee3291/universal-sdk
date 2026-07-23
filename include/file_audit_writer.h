#ifndef FILE_AUDIT_WRITER_H
#define FILE_AUDIT_WRITER_H

#include "legal_assistant.h"

#include <fstream>
#include <mutex>
#include <string>

namespace UniversalSDK {

// Appends each AuditEvent as an NDJSON (newline-delimited JSON) line to a
// log file.  Thread-safe: concurrent Write() calls are serialized internally.
//
// Example line format:
//   {"audit_id":"audit-1","timestamp_utc":"2026-07-21T20:00:00Z",
//    "action":"advice_generated","details":"Policy-approved response..."}
class FileAuditWriter : public AuditWriter {
public:
    // Opens (or creates) the file at file_path in append mode.
    // Throws std::runtime_error if the file cannot be opened.
    explicit FileAuditWriter(const std::string& file_path);

    ~FileAuditWriter() override;

    // Thread-safe: serializes concurrent writes.
    void Write(const AuditEvent& event) override;

private:
    static std::string EscapeJsonString(const std::string& s);

    std::ofstream file_;
    std::mutex mutex_;
};

} // namespace UniversalSDK

#endif // FILE_AUDIT_WRITER_H
