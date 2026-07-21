#include "file_audit_writer.h"

#include <sstream>
#include <stdexcept>

namespace UniversalSDK {

FileAuditWriter::FileAuditWriter(const std::string& file_path)
    : file_(file_path, std::ios::app) {
    if (!file_.is_open()) {
        throw std::runtime_error("FileAuditWriter: cannot open log file: " + file_path);
    }
}

FileAuditWriter::~FileAuditWriter() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void FileAuditWriter::Write(const AuditEvent& event) {
    std::ostringstream line;
    line << "{"
         << "\"audit_id\":\"" << EscapeJsonString(event.audit_id) << "\","
         << "\"timestamp_utc\":\"" << EscapeJsonString(event.timestamp_utc) << "\","
         << "\"action\":\"" << EscapeJsonString(event.action) << "\","
         << "\"details\":\"" << EscapeJsonString(event.details) << "\""
         << "}\n";

    std::lock_guard<std::mutex> lock(mutex_);
    file_ << line.str();
    file_.flush();
}

// Escapes the minimal set of characters required to produce valid JSON strings.
std::string FileAuditWriter::EscapeJsonString(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (const char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;     break;
        }
    }
    return out;
}

} // namespace UniversalSDK
