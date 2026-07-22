package com.universalsdk.legalassist;

/**
 * JNI wrapper for the UniversalSDK legal-assistant native library.
 * The native methods are implemented in jni_bridge.cpp.
 */
public class LegalAssistantJNI {

    static {
        System.loadLibrary("legal_assist_jni");
    }

    /**
     * Submits a legal query to the SDK and returns a JSON string containing the
     * {@link LegalAdviceResult} fields.
     *
     * @param inmateId       Inmate record identifier (e.g. "I-001").
     * @param jurisdiction   Jurisdiction code (e.g. "CA").
     * @param caseType       Case type (e.g. "appeal", "parole").
     * @param question       Free-form legal question from the requester.
     * @param caseSummary    Brief background for the inmate's case.
     * @param consentGranted Whether the inmate has given consent for data use.
     * @return JSON-encoded {@link LegalAdviceResult}.
     */
    public static native String queryLegalAdvice(
            String inmateId,
            String jurisdiction,
            String caseType,
            String question,
            String caseSummary,
            boolean consentGranted);

    /**
     * Returns {@code true} if the native library loaded and the SDK is available.
     */
    public static native boolean isSDKAvailable();
}
