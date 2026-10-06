import android.app.Activity;
import android.os.Build;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;
import org.gbarecomp.BackNavigation;

public final class AndroidBackNavigationTest {
    private static final class Dispatcher implements OnBackInvokedDispatcher {
        OnBackInvokedCallback callback;
        int registrations;
        int removals;
        public void registerOnBackInvokedCallback(int priority, OnBackInvokedCallback value) {
            check(priority == PRIORITY_DEFAULT, "Correct Back priority");
            check(callback == null, "No duplicate registration");
            callback = value;
            ++registrations;
        }
        public void unregisterOnBackInvokedCallback(OnBackInvokedCallback value) {
            check(callback == value, "Exact callback removed");
            callback = null;
            ++removals;
        }
    }

    private static void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
    }

    public static void main(String[] args) {
        Activity activity = new Activity();
        Dispatcher dispatcher = new Dispatcher();
        activity.dispatcher = dispatcher;
        int[] presses = {0};
        for (int sdk : new int[]{28, 29, 30, 31, 32}) {
            Build.VERSION.SDK_INT = sdk;
            BackNavigation.register(activity, () -> ++presses[0]).run();
        }
        check(dispatcher.registrations == 0, "Legacy Android does not use API 33");
        for (int sdk : new int[]{33, 34, 35, 36}) {
            Build.VERSION.SDK_INT = sdk;
            int previous = presses[0];
            Runnable cleanup = BackNavigation.register(activity, () -> ++presses[0]);
            check(presses[0] == previous, "Registration does not trigger Back");
            dispatcher.callback.onBackInvoked();
            check(presses[0] == previous + 1, "One system Back invokes one action");
            cleanup.run();
            check(dispatcher.callback == null, "Paused/export Activity has no game callback");
        }
        check(dispatcher.registrations == dispatcher.removals, "No callbacks leaked across resumes");
        System.out.println("PASS: modern Back, one action, lifecycle cleanup and legacy Android guard");
    }
}
