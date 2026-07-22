package com.universalsdk.legalassist;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import org.json.JSONException;

import java.util.List;

/**
 * Main screen of the Inmate Legal Support app.
 *
 * The user enters a query (inmate ID, jurisdiction, case type, question, and an
 * optional case summary), taps "Get Legal Advice", and the SDK response is
 * displayed in a scrollable results panel.
 */
public class MainActivity extends AppCompatActivity {

    private EditText  editInmateId;
    private EditText  editJurisdiction;
    private EditText  editCaseType;
    private EditText  editQuestion;
    private EditText  editCaseSummary;
    private CheckBox  checkConsent;
    private Button    btnSubmit;
    private ProgressBar progressBar;
    private ScrollView  scrollRoot;
    private TextView    tvResults;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editInmateId    = findViewById(R.id.editInmateId);
        editJurisdiction = findViewById(R.id.editJurisdiction);
        editCaseType    = findViewById(R.id.editCaseType);
        editQuestion    = findViewById(R.id.editQuestion);
        editCaseSummary = findViewById(R.id.editCaseSummary);
        checkConsent    = findViewById(R.id.checkConsent);
        btnSubmit       = findViewById(R.id.btnSubmit);
        progressBar     = findViewById(R.id.progressBar);
        scrollRoot      = findViewById(R.id.scrollRoot);
        tvResults       = findViewById(R.id.tvResults);

        btnSubmit.setOnClickListener(v -> submitQuery());
    }

    private void submitQuery() {
        String inmateId    = editInmateId.getText().toString().trim();
        String jurisdiction = editJurisdiction.getText().toString().trim();
        String caseType    = editCaseType.getText().toString().trim();
        String question    = editQuestion.getText().toString().trim();
        String caseSummary = editCaseSummary.getText().toString().trim();
        boolean consent    = checkConsent.isChecked();

        if (TextUtils.isEmpty(inmateId) || TextUtils.isEmpty(jurisdiction)
                || TextUtils.isEmpty(caseType) || TextUtils.isEmpty(question)) {
            Toast.makeText(this,
                    getString(R.string.error_required_fields),
                    Toast.LENGTH_SHORT).show();
            return;
        }

        if (!consent) {
            Toast.makeText(this,
                    getString(R.string.error_consent_required),
                    Toast.LENGTH_SHORT).show();
            return;
        }

        btnSubmit.setEnabled(false);
        progressBar.setVisibility(View.VISIBLE);
        tvResults.setVisibility(View.GONE);

        // Run the native call on a background thread to keep the UI responsive.
        new Thread(() -> {
            String json = LegalAssistantJNI.queryLegalAdvice(
                    inmateId, jurisdiction, caseType, question, caseSummary, consent);

            runOnUiThread(() -> {
                progressBar.setVisibility(View.GONE);
                btnSubmit.setEnabled(true);
                try {
                    LegalAdviceResult result = LegalAdviceResult.fromJson(json);
                    displayResult(result);
                } catch (JSONException e) {
                    tvResults.setText(getString(R.string.error_parse_failed) + "\n" + e.getMessage());
                    tvResults.setVisibility(View.VISIBLE);
                }
            });
        }).start();
    }

    private void displayResult(LegalAdviceResult result) {
        StringBuilder sb = new StringBuilder();

        sb.append("Audit ID: ").append(result.auditId).append("\n\n");

        if (result.escalated) {
            sb.append("⚠ ESCALATED TO HUMAN REVIEW\n");
            if (!TextUtils.isEmpty(result.escalationReason)) {
                sb.append("Reason: ").append(result.escalationReason).append("\n");
            }
            sb.append("\n");
        }

        if (!TextUtils.isEmpty(result.summary)) {
            sb.append("── Summary ──────────────────────────\n");
            sb.append(result.summary).append("\n\n");
        }

        appendSection(sb, "Guidance Steps", result.guidanceSteps, true);
        appendSection(sb, "Citations",      result.citations,     true);
        appendSection(sb, "Disclaimers",    result.disclaimers,   false);

        tvResults.setText(sb.toString().trim());
        tvResults.setVisibility(View.VISIBLE);
        scrollRoot.post(() -> scrollRoot.smoothScrollTo(0, tvResults.getTop()));
    }

    private void appendSection(
            StringBuilder sb, String header, List<String> items, boolean numbered) {
        if (items == null || items.isEmpty()) return;
        sb.append("── ").append(header).append(" ──────────────────────────\n");
        for (int i = 0; i < items.size(); i++) {
            if (numbered) {
                sb.append(i + 1).append(". ").append(items.get(i)).append("\n");
            } else {
                sb.append("• ").append(items.get(i)).append("\n");
            }
        }
        sb.append("\n");
    }
}
