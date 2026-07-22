package com.universalsdk.legalassist;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Parsed representation of a legal-advice response returned from the native SDK.
 */
public class LegalAdviceResult {

    public final boolean escalated;
    public final String  escalationReason;
    public final String  summary;
    public final double  confidence;
    public final String  auditId;
    public final List<String> guidanceSteps;
    public final List<String> disclaimers;
    public final List<String> citations;

    private LegalAdviceResult(
            boolean escalated,
            String  escalationReason,
            String  summary,
            double  confidence,
            String  auditId,
            List<String> guidanceSteps,
            List<String> disclaimers,
            List<String> citations) {
        this.escalated        = escalated;
        this.escalationReason = escalationReason;
        this.summary          = summary;
        this.confidence       = confidence;
        this.auditId          = auditId;
        this.guidanceSteps    = Collections.unmodifiableList(guidanceSteps);
        this.disclaimers      = Collections.unmodifiableList(disclaimers);
        this.citations        = Collections.unmodifiableList(citations);
    }

    /**
     * Parses the JSON string produced by {@link LegalAssistantJNI#queryLegalAdvice}.
     *
     * @throws JSONException if the JSON is malformed.
     */
    public static LegalAdviceResult fromJson(String json) throws JSONException {
        JSONObject obj = new JSONObject(json);

        boolean escalated        = obj.optBoolean("escalated", true);
        String  escalationReason = obj.optString("escalation_reason", "");
        String  summary          = obj.optString("summary", "");
        double  confidence       = obj.optDouble("confidence", 0.0);
        String  auditId          = obj.optString("audit_id", "");

        List<String> guidanceSteps = parseStringArray(obj.optJSONArray("guidance_steps"));
        List<String> disclaimers   = parseStringArray(obj.optJSONArray("disclaimers"));
        List<String> citations     = parseCitationArray(obj.optJSONArray("citations"));

        return new LegalAdviceResult(
                escalated, escalationReason, summary, confidence, auditId,
                guidanceSteps, disclaimers, citations);
    }

    private static List<String> parseStringArray(JSONArray arr) throws JSONException {
        List<String> list = new ArrayList<>();
        if (arr == null) return list;
        for (int i = 0; i < arr.length(); i++) {
            list.add(arr.getString(i));
        }
        return list;
    }

    private static List<String> parseCitationArray(JSONArray arr) throws JSONException {
        List<String> list = new ArrayList<>();
        if (arr == null) return list;
        for (int i = 0; i < arr.length(); i++) {
            JSONObject c = arr.getJSONObject(i);
            String ref   = c.optString("reference", "");
            String title = c.optString("title", "");
            String src   = c.optString("source", "");
            list.add("[" + ref + "] " + title + " (" + src + ")");
        }
        return list;
    }
}
