package org.gbarecomp;

import android.app.Activity;
import android.os.Build;

/** Registers modern Back only while the game is the resumed Activity. */
public final class BackNavigation {
    private BackNavigation() {}

    public static Runnable register(Activity activity, Runnable action) {
        if (Build.VERSION.SDK_INT < 33) return () -> {};
        return Api33.register(activity, action);
    }

    // Isolate API 33 types so Android 9-12 can keep using the legacy handlers.
    private static final class Api33 {
        static Runnable register(Activity activity, Runnable action) {
            android.window.OnBackInvokedDispatcher dispatcher = activity.getOnBackInvokedDispatcher();
            android.window.OnBackInvokedCallback callback = action::run;
            dispatcher.registerOnBackInvokedCallback(
                android.window.OnBackInvokedDispatcher.PRIORITY_DEFAULT, callback);
            return () -> dispatcher.unregisterOnBackInvokedCallback(callback);
        }
    }
}
