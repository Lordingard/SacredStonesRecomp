package org.gbarecomp;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.TextView;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Keeps the game Activity paused while the archive is prepared and written. */
public final class SaveExportActivity extends Activity {
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private boolean exporting;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView status = new TextView(this);
        status.setText("Export saves");
        status.setTextSize(20);
        status.setGravity(Gravity.CENTER);
        setContentView(status);
        if (savedInstanceState == null) {
            Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/zip");
            intent.putExtra(Intent.EXTRA_TITLE, "SacredStonesRecomp-saves.zip");
            startActivityForResult(intent, 1);
        } else if (savedInstanceState.getBoolean("exporting")) {
            result("Export interrupted", "No completed backup was confirmed. Please try again.");
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle state) {
        state.putBoolean("exporting", exporting);
        super.onSaveInstanceState(state);
    }

    @Override
    public void onBackPressed() {
        if (!exporting) finish();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != 1) return;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            finish();
            return;
        }
        Uri uri = data.getData();
        exporting = true;
        worker.execute(() -> {
            File staged = null;
            try {
                staged = File.createTempFile("save-export-", ".zip", getCacheDir());
                int count;
                try (FileOutputStream output = new FileOutputStream(staged)) {
                    count = SaveArchive.write(getFilesDir(), output);
                }
                try (InputStream input = new FileInputStream(staged);
                     OutputStream output = getContentResolver().openOutputStream(uri, "wt")) {
                    if (output == null) throw new IOException("The destination cannot be written.");
                    byte[] buffer = new byte[65536];
                    int read;
                    while ((read = input.read(buffer)) != -1) output.write(buffer, 0, read);
                }
                final int exported = count;
                runOnUiThread(() -> result("Saves exported", "Exported " + exported + " save files."));
            } catch (Exception error) {
                runOnUiThread(() -> result("Export failed", error.getMessage()));
            } finally {
                if (staged != null) staged.delete();
            }
        });
    }

    private void result(String title, String message) {
        if (isDestroyed() || isFinishing()) return;
        exporting = false;
        new AlertDialog.Builder(this).setTitle(title).setMessage(message)
            .setCancelable(false).setPositiveButton("OK", (dialog, which) -> finish()).show();
    }

    @Override
    protected void onDestroy() {
        worker.shutdownNow();
        super.onDestroy();
    }
}
