package org.gbarecomp;

import android.content.Context;
import android.util.AtomicFile;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

/** Shared single-byte preference used by setup and the native in-game menu. */
public final class TouchControls {
    private TouchControls() {}
    public static boolean enabled(Context context) {
        File file = new File(context.getFilesDir(), "touch-controls.cfg");
        if (!file.exists()) return true;
        try (FileInputStream input = new FileInputStream(file)) {
            return input.read() == '1';
        } catch (IOException error) { return false; }
    }
    public static boolean save(Context context, boolean enabled) {
        AtomicFile file = new AtomicFile(new File(context.getFilesDir(), "touch-controls.cfg"));
        FileOutputStream output = null;
        try {
            output = file.startWrite();
            output.write(enabled ? '1' : '0');
            file.finishWrite(output);
            return true;
        } catch (IOException error) {
            if (output != null) file.failWrite(output);
            return false;
        }
    }
}
