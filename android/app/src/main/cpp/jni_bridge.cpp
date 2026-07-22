// jni_bridge.cpp
//
// JNI layer that exposes the UniversalSDK legal-assistant module to Java.
// The native library is loaded by LegalAssistantJNI.java.

#include <jni.h>
#include <android/log.h>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "legal_assistant.h"
#include "stub_providers.h"
#include "universal_sdk.h"

#define LOG_TAG "LegalAssistJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace UniversalSDK;

namespace {

// Converts a jstring to a std::string, returning "" for null.
std::string JstrToStd(JNIEnv* env, jstring js) {
    if (!js) return "";
    const char* cs = env->GetStringUTFChars(js, nullptr);
    if (!cs) return "";
    std::string s(cs);
    env->ReleaseStringUTFChars(js, cs);
    return s;
}

// JSON string escaper.
std::string EscapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    std::ostringstream escaped;
                    escaped << "\\u"
                            << std::hex << std::uppercase << std::setw(4)
                            << std::setfill('0') << static_cast<int>(c);
                    out += escaped.str();
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Java_com_universalsdk_legalassist_LegalAssistantJNI_queryLegalAdvice
//
// Parameters (all from Java):
//   inmateId       - inmate record identifier
//   jurisdiction   - e.g. "CA"
//   caseType       - e.g. "appeal"
//   question       - free-form legal question
//   caseSummary    - brief background for the inmate's case
//   consentGranted - whether the inmate has given consent
//
// Returns a JSON object string with fields:
//   escalated, escalation_reason, summary, confidence, audit_id,
//   guidance_steps[], disclaimers[], citations[]
// ---------------------------------------------------------------------------
extern "C" JNIEXPORT jstring JNICALL
Java_com_universalsdk_legalassist_LegalAssistantJNI_queryLegalAdvice(
        JNIEnv* env,
        jclass  /* clazz */,
        jstring inmateId,
        jstring jurisdiction,
        jstring caseType,
        jstring question,
        jstring caseSummary,
        jboolean consentGranted) {

    const std::string inmateIdStr     = JstrToStd(env, inmateId);
    const std::string jurisdictionStr = JstrToStd(env, jurisdiction);
    const std::string caseTypeStr     = JstrToStd(env, caseType);
    const std::string questionStr     = JstrToStd(env, question);
    const std::string caseSummaryStr  = JstrToStd(env, caseSummary);

    LOGI("queryLegalAdvice request received");

    try {
        // Wire up the in-memory providers.
        auto connector = std::make_shared<InMemoryInmateDataConnector>();
        connector->AddInmate(
            inmateIdStr,
            caseSummaryStr.empty() ? "No case summary provided." : caseSummaryStr);
        connector->GrantAccess(inmateIdStr, AccessRole::LEGAL_ADVOCATE);
        connector->GrantAccess(inmateIdStr, AccessRole::INMATE);

        // EchoLLMProvider reflects the prompt back; swap for a real model in
        // production.
        auto llm = std::make_shared<EchoLLMProvider>(/*confidence=*/0.9);

        // Seed a few generic citations; a production build would query a real
        // legal-document store.
        std::vector<Citation> citations = {
            {"Legal Aid Society",   "Inmate Rights Overview",          "LAS-001"},
            {"State Penal Code",    "Appeal Procedures",               "SPC-APP"},
            {"Federal Rules",       "Habeas Corpus – 28 U.S.C. §2254", "FED-2254"},
        };
        auto retrieval = std::make_shared<InMemoryRetrievalProvider>(citations);

        LegalAssistantConfig config;
        config.allowed_jurisdictions = {jurisdictionStr};
        config.allowed_case_types    = {caseTypeStr};
        config.confidence_threshold  = 0.75;

        LegalAssistant assistant(
            llm, retrieval, connector,
            std::make_shared<DefaultPolicyEngine>(),
            config);

        LegalQueryContext ctx;
        ctx.inmate_id       = inmateIdStr;
        ctx.jurisdiction    = jurisdictionStr;
        ctx.case_type       = caseTypeStr;
        ctx.question        = questionStr;
        ctx.role            = AccessRole::LEGAL_ADVOCATE;
        ctx.consent_granted = (consentGranted == JNI_TRUE);

        const LegalAdviceResponse resp = assistant.Advise(ctx);

        // Serialise response as JSON.
        std::ostringstream json;
        json << "{";
        json << "\"escalated\":"         << (resp.escalated ? "true" : "false") << ",";
        json << "\"escalation_reason\":\"" << EscapeJson(resp.escalation_reason) << "\",";
        json << "\"summary\":\""         << EscapeJson(resp.summary)            << "\",";
        json << "\"confidence\":"        << resp.confidence                     << ",";
        json << "\"audit_id\":\""        << EscapeJson(resp.audit_id)           << "\",";

        json << "\"guidance_steps\":[";
        for (size_t i = 0; i < resp.guidance_steps.size(); ++i) {
            if (i) json << ",";
            json << "\"" << EscapeJson(resp.guidance_steps[i]) << "\"";
        }
        json << "],";

        json << "\"disclaimers\":[";
        for (size_t i = 0; i < resp.disclaimers.size(); ++i) {
            if (i) json << ",";
            json << "\"" << EscapeJson(resp.disclaimers[i]) << "\"";
        }
        json << "],";

        json << "\"citations\":[";
        for (size_t i = 0; i < resp.citations.size(); ++i) {
            if (i) json << ",";
            json << "{"
                 << "\"source\":\""    << EscapeJson(resp.citations[i].source)    << "\","
                 << "\"title\":\""     << EscapeJson(resp.citations[i].title)     << "\","
                 << "\"reference\":\"" << EscapeJson(resp.citations[i].reference) << "\""
                 << "}";
        }
        json << "]";

        json << "}";

        return env->NewStringUTF(json.str().c_str());

    } catch (const std::exception& ex) {
        LOGE("queryLegalAdvice exception: %s", ex.what());
        std::ostringstream err;
        err << "{\"escalated\":true,\"escalation_reason\":\""
            << EscapeJson(std::string("Internal error: ") + ex.what())
            << "\",\"summary\":\"\",\"confidence\":0,"
            << "\"audit_id\":\"\",\"guidance_steps\":[],\"disclaimers\":[],\"citations\":[]}";
        return env->NewStringUTF(err.str().c_str());
    }
}

// Returns true – confirms the native library loaded successfully.
extern "C" JNIEXPORT jboolean JNICALL
Java_com_universalsdk_legalassist_LegalAssistantJNI_isSDKAvailable(
        JNIEnv* /* env */,
        jclass  /* clazz */) {
    return JNI_TRUE;
}
